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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tmux.h"

static struct screen	*window_keys_init(struct window_mode_entry *,
			     struct cmd_find_state *, struct args *);
static void		 window_keys_free(struct window_mode_entry *);
static void		 window_keys_resize(struct window_mode_entry *, u_int,
			     u_int);
static void		 window_keys_update(struct window_mode_entry *);
static void		 window_keys_key(struct window_mode_entry *,
			     struct client *, struct session *,
			     struct winlink *, key_code, struct mouse_event *);

#define WINDOW_KEYS_DEFAULT_FORMAT \
	"#{key_binding_key}: #{key_binding_note}"

static const struct menu_item window_keys_menu_items[] = {
	{ "Cancel", 'q', NULL },
	{ NULL, KEYC_NONE, NULL }
};

const struct window_mode window_keys_mode = {
	.name = "keys-mode",
	.default_format = WINDOW_KEYS_DEFAULT_FORMAT,

	.init = window_keys_init,
	.free = window_keys_free,
	.resize = window_keys_resize,
	.update = window_keys_update,
	.key = window_keys_key,
};

struct window_keys_itemdata {
	const char		*key_str;
	const char		*note;
	const char		*command;
	const char		*table_name;
	enum keys_category	 category;
	key_code		 key;
};

struct window_keys_modedata {
	struct window_pane		 *wp;
	struct cmd_find_state		  fs;

	struct mode_tree_data		 *data;
	char				 *format;

	struct window_keys_itemdata	**item_list;
	u_int				  item_size;
};

static struct window_keys_itemdata *
window_keys_add_item(struct window_keys_modedata *data)
{
	struct window_keys_itemdata	*item;

	data->item_list = xreallocarray(data->item_list, data->item_size + 1,
	    sizeof *data->item_list);
	item = data->item_list[data->item_size++] = xcalloc(1, sizeof *item);
	return (item);
}

static void
window_keys_free_item(struct window_keys_itemdata *item)
{
	free((void *)item->key_str);
	free((void *)item->note);
	free((void *)item->command);
	free((void *)item->table_name);
	free(item);
}

static void
window_keys_build(void *modedata, __unused struct mode_tree_sort_criteria *sort_crit,
    __unused uint64_t *tag, const char *filter)
{
	struct window_keys_modedata	*data = modedata;
	struct window_keys_itemdata	*item;
	struct key_table		*table;
	struct key_binding		*bd;
	u_int				 i;
	char				*text, *cp;
	struct format_tree		*ft;

	log_debug("window_keys_build: starting");

	for (i = 0; i < data->item_size; i++)
		window_keys_free_item(data->item_list[i]);
	free(data->item_list);
	data->item_list = NULL;
	data->item_size = 0;

	/* Build list of all keybindings */
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

			item = window_keys_add_item(data);
			item->key = bd->key;
			item->key_str = xstrdup(key_string_lookup_key(bd->key, 0));
			item->note = xstrdup(bd->note);
			cp = cmd_list_print(bd->cmdlist, 0);
			item->command = xstrdup(cp);
			item->table_name = xstrdup(table->name);
			item->category = keys_category_match(cp, bd->note);
			free(cp);

			bd = key_bindings_next(table, bd);
		}
		table = key_bindings_next_table(table);
	}

	log_debug("window_keys_build: built %u items", data->item_size);

	/* Add items to mode tree */
	for (i = 0; i < data->item_size; i++) {
		item = data->item_list[i];

		ft = format_create(NULL, NULL, FORMAT_NONE, 0);
		format_add(ft, "key_binding_key", "%s", item->key_str);
		format_add(ft, "key_binding_note", "%s", item->note);
		format_add(ft, "key_binding_command", "%s", item->command);
		format_add(ft, "key_binding_table", "%s", item->table_name);
		format_add(ft, "key_binding_category", "%s",
		    keys_category_name(item->category));

		if (filter != NULL) {
			cp = format_expand(ft, filter);
			if (!format_true(cp)) {
				free(cp);
				format_free(ft);
				continue;
			}
			free(cp);
		}

		text = format_expand(ft, data->format);
		mode_tree_add(data->data, NULL, item, i, item->key_str,
		    text, -1);
		free(text);

		format_free(ft);
	}

	log_debug("window_keys_build: done");
}

static void
window_keys_draw(__unused void *modedata, void *itemdata,
    struct screen_write_ctx *ctx, u_int sx, u_int sy)
{
	struct window_keys_itemdata	*item = itemdata;
	char				*line;
	u_int				 i, cx = ctx->s->cx, cy = ctx->s->cy;

	/* Show key, note, and command */
	i = 0;

	if (i < sy) {
		xasprintf(&line, "Key:      %s", item->key_str);
		screen_write_cursormove(ctx, cx, cy + i, 0);
		screen_write_nputs(ctx, sx, &grid_default_cell, "%s", line);
		free(line);
		i++;
	}

	if (i < sy) {
		xasprintf(&line, "Note:     %s", item->note);
		screen_write_cursormove(ctx, cx, cy + i, 0);
		screen_write_nputs(ctx, sx, &grid_default_cell, "%s", line);
		free(line);
		i++;
	}

	if (i < sy) {
		xasprintf(&line, "Command:  %s", item->command);
		screen_write_cursormove(ctx, cx, cy + i, 0);
		screen_write_nputs(ctx, sx, &grid_default_cell, "%s", line);
		free(line);
		i++;
	}

	if (i < sy) {
		xasprintf(&line, "Table:    %s", item->table_name);
		screen_write_cursormove(ctx, cx, cy + i, 0);
		screen_write_nputs(ctx, sx, &grid_default_cell, "%s", line);
		free(line);
		i++;
	}

	if (i < sy) {
		xasprintf(&line, "Category: %s", keys_category_name(item->category));
		screen_write_cursormove(ctx, cx, cy + i, 0);
		screen_write_nputs(ctx, sx, &grid_default_cell, "%s", line);
		free(line);
		i++;
	}
}

static int
window_keys_search(__unused void *modedata, void *itemdata, const char *ss)
{
	struct window_keys_itemdata	*item = itemdata;

	if (strstr(item->key_str, ss) != NULL)
		return (1);
	if (strstr(item->note, ss) != NULL)
		return (1);
	if (strstr(item->command, ss) != NULL)
		return (1);
	return (0);
}

static void
window_keys_menu(void *modedata, struct client *c, key_code key)
{
	struct window_keys_modedata	*data = modedata;
	struct window_pane		*wp = data->wp;
	struct window_mode_entry	*wme;

	wme = TAILQ_FIRST(&wp->modes);
	if (wme == NULL || wme->data != modedata)
		return;
	window_keys_key(wme, c, NULL, NULL, key, NULL);
}

static struct screen *
window_keys_init(struct window_mode_entry *wme, struct cmd_find_state *fs,
    struct args *args)
{
	struct window_pane		*wp = wme->wp;
	struct window_keys_modedata	*data;
	struct screen			*s;

	log_debug("window_keys_init: starting");

	wme->data = data = xcalloc(1, sizeof *data);
	data->wp = wp;
	cmd_find_copy_state(&data->fs, fs);

	if (args == NULL || !args_has(args, 'F'))
		data->format = xstrdup(WINDOW_KEYS_DEFAULT_FORMAT);
	else
		data->format = xstrdup(args_get(args, 'F'));

	data->data = mode_tree_start(wp, args, window_keys_build,
	    window_keys_draw, window_keys_search, window_keys_menu, NULL,
	    NULL, NULL, data, window_keys_menu_items, NULL, 0, &s);
	mode_tree_zoom(data->data, args);

	mode_tree_build(data->data);
	mode_tree_draw(data->data);

	log_debug("window_keys_init: done");
	return (s);
}

static void
window_keys_free(struct window_mode_entry *wme)
{
	struct window_keys_modedata	*data = wme->data;
	u_int				 i;

	if (data == NULL)
		return;

	mode_tree_free(data->data);

	for (i = 0; i < data->item_size; i++)
		window_keys_free_item(data->item_list[i]);
	free(data->item_list);

	free(data->format);

	free(data);
}

static void
window_keys_resize(struct window_mode_entry *wme, u_int sx, u_int sy)
{
	struct window_keys_modedata	*data = wme->data;

	mode_tree_resize(data->data, sx, sy);
}

static void
window_keys_update(struct window_mode_entry *wme)
{
	struct window_keys_modedata	*data = wme->data;

	mode_tree_build(data->data);
	mode_tree_draw(data->data);
	data->wp->flags |= PANE_REDRAW;
}

static void
window_keys_key(struct window_mode_entry *wme, struct client *c,
    __unused struct session *s, __unused struct winlink *wl, key_code key,
    struct mouse_event *m)
{
	struct window_pane		*wp = wme->wp;
	struct window_keys_modedata	*data = wme->data;
	struct mode_tree_data		*mtd = data->data;
	int				 finished;

	finished = mode_tree_key(mtd, c, &key, m, NULL, NULL);

	if (finished)
		window_pane_reset_mode(wp);
	else {
		mode_tree_draw(mtd);
		wp->flags |= PANE_REDRAW;
	}
}
