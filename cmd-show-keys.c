/* $OpenBSD$ */

/*
 * Copyright (c) 2025 Swarm Code Contributors
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tmux.h"

/*
 * Show keybindings in an interactive cheatsheet.
 */

static enum cmd_retval	cmd_show_keys_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_show_keys_entry = {
	.name = "show-keys",
	.alias = "keys",

	.args = { "F:f:swpP", 0, 0, NULL },
	.usage = "[-swpP] [-F format] [-f filter]",

	.flags = CMD_STARTSERVER|CMD_AFTERHOOK,
	.exec = cmd_show_keys_exec
};

static enum cmd_retval
cmd_show_keys_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = cmd_get_args(self);
	struct key_table	*table;
	struct key_binding	*bd;
	const char		*filter = NULL;
	char			*cp, *line, *markdown, *tmpfile, *cmd_str;
	enum keys_category	 cat, filter_cat = KEYS_CAT_MISC;
	int			 show_all = 1, fd;
	FILE			*fp;
	struct cmd_parse_input	 pi;
	enum cmd_parse_status	 status;

	/* Determine filter category if specified */
	if (args_has(args, 'p')) {
		filter_cat = KEYS_CAT_PANE;
		show_all = 0;
	}
	if (args_has(args, 'w')) {
		filter_cat = KEYS_CAT_WINDOW;
		show_all = 0;
	}
	if (args_has(args, 's')) {
		filter_cat = KEYS_CAT_SESSION;
		show_all = 0;
	}
	if (args_has(args, 'P')) {
		filter_cat = KEYS_CAT_PROJECT;
		show_all = 0;
	}

	/* Get custom filter if provided */
	if (args_has(args, 'f'))
		filter = args_get(args, 'f');

	/* Simple mode: just print keybinds */
	if (args_has(args, 'F')) {
		const char *format = args_get(args, 'F');
		struct format_tree *ft;

		ft = format_create(cmdq_get_client(item), item, FORMAT_NONE, 0);
		format_defaults(ft, NULL, NULL, NULL, NULL);

		table = key_bindings_first_table();
		while (table != NULL) {
			bd = key_bindings_first(table);
			while (bd != NULL) {
				if (KEYC_IS_MOUSE(bd->key) ||
				    bd->note == NULL ||
				    *bd->note == '\0') {
					bd = key_bindings_next(table, bd);
					continue;
				}

				/* Get command string */
				cp = cmd_list_print(bd->cmdlist, 0);
				if (cp == NULL) {
					bd = key_bindings_next(table, bd);
					continue;
				}

				/* Classify into category */
				cat = keys_category_match(cp, bd->note);

				/* Apply filter if specified */
				if (!show_all && cat != filter_cat) {
					free(cp);
					bd = key_bindings_next(table, bd);
					continue;
				}

				/* Apply custom filter if specified */
				if (filter != NULL) {
					if (strcasestr(bd->note, filter) == NULL &&
					    strcasestr(cp, filter) == NULL) {
						free(cp);
						bd = key_bindings_next(table, bd);
						continue;
					}
				}

				/* Format and print */
				format_add(ft, "key", "%s",
				    key_string_lookup_key(bd->key, 0));
				format_add(ft, "note", "%s", bd->note);
				format_add(ft, "command", "%s", cp);
				format_add(ft, "table", "%s", table->name);
				format_add(ft, "category", "%s",
				    keys_category_name(cat));

				line = format_expand(ft, format);
				cmdq_print(item, "%s", line);
				free(line);
				free(cp);

				bd = key_bindings_next(table, bd);
			}
			table = key_bindings_next_table(table);
		}
		format_free(ft);
		return (CMD_RETURN_NORMAL);
	}

	/* Default mode: open interactive markdown cheatsheet */

	/* Generate markdown cheatsheet */
	markdown = keys_generate_markdown_cheatsheet();
	if (markdown == NULL) {
		cmdq_error(item, "failed to generate cheatsheet");
		return (CMD_RETURN_ERROR);
	}

	/* Check if we have any active sessions */
	if (RB_EMPTY(&sessions)) {
		/* No sessions, just print markdown to stdout */
		cmdq_print(item, "%s", markdown);
		free(markdown);
		return (CMD_RETURN_NORMAL);
	}

	/* Write to temporary file */
	xasprintf(&tmpfile, "/tmp/smux-keys-%d.md", getpid());
	fd = open(tmpfile, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	if (fd == -1) {
		cmdq_error(item, "failed to create temp file");
		free(markdown);
		free(tmpfile);
		return (CMD_RETURN_ERROR);
	}

	fp = fdopen(fd, "w");
	if (fp == NULL) {
		close(fd);
		cmdq_error(item, "failed to open temp file");
		free(markdown);
		free(tmpfile);
		return (CMD_RETURN_ERROR);
	}

	fprintf(fp, "%s", markdown);
	fclose(fp);
	free(markdown);

	/* Open in new window with less for vim-style navigation */
	xasprintf(&cmd_str, "new-window -n 'Keybindings' 'less -R %s; rm %s'",
	    tmpfile, tmpfile);

	memset(&pi, 0, sizeof pi);
	status = cmd_parse_and_insert(cmd_str, &pi, item, NULL, NULL);
	free(cmd_str);
	free(tmpfile);

	if (status == CMD_PARSE_ERROR)
		return (CMD_RETURN_ERROR);
	return (CMD_RETURN_NORMAL);
}
