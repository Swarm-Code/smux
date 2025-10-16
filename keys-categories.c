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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "tmux.h"

/* Category definitions with icons and colors */
static const struct keys_category_def categories[] = {
	{ KEYS_CAT_PROJECT, "Project Management", "📁", "#95E6CB",
	  "Create, switch, and manage projects" },
	{ KEYS_CAT_SESSION, "Session Operations", "🖥️ ", "#E6A95E",
	  "Attach, detach, and switch sessions" },
	{ KEYS_CAT_WINDOW, "Window Management", "🪟", "#D19A66",
	  "Create, rename, and navigate windows" },
	{ KEYS_CAT_PANE, "Pane Operations", "📋", "#61AFEF",
	  "Split, resize, and manage panes" },
	{ KEYS_CAT_COPY, "Copy Mode", "📄", "#C678DD",
	  "Copy, paste, and buffer operations" },
	{ KEYS_CAT_NAVIGATION, "Navigation", "🧭", "#98C379",
	  "Movement and selection commands" },
	{ KEYS_CAT_LAYOUT, "Layout", "🗂️ ", "#56B6C2",
	  "Arrange panes and windows" },
	{ KEYS_CAT_MISC, "Miscellaneous", "⚙️ ", "#ABB2BF",
	  "Other commands and utilities" },
	{ KEYS_CAT_MOUSE, "Mouse Bindings", "🖱️ ", "#E5C07B",
	  "Mouse click and drag operations" }
};

/* Keywords for category matching */
static const struct {
	enum keys_category cat;
	const char *keywords[12];
} category_keywords[] = {
	{ KEYS_CAT_PROJECT, {
		"project", "new-project", "list-project", "kill-project",
		"rename-project", "switch-project", NULL
	}},
	{ KEYS_CAT_SESSION, {
		"session", "attach", "detach", "switch-client", "choose-client",
		"new-session", "kill-session", "rename-session", NULL
	}},
	{ KEYS_CAT_WINDOW, {
		"window", "new-window", "kill-window", "rename-window",
		"next-window", "previous-window", "select-window", "move-window",
		"swap-window", "find-window", NULL
	}},
	{ KEYS_CAT_PANE, {
		"pane", "split-window", "kill-pane", "select-pane", "swap-pane",
		"resize-pane", "break-pane", "join-pane", "display-pane", NULL
	}},
	{ KEYS_CAT_COPY, {
		"copy", "paste", "buffer", "copy-mode", "list-buffer",
		"delete-buffer", "set-buffer", "show-buffer", "save-buffer",
		"load-buffer", NULL
	}},
	{ KEYS_CAT_NAVIGATION, {
		"select", "last-", "next", "previous", "choose-tree",
		"choose-buffer", NULL
	}},
	{ KEYS_CAT_LAYOUT, {
		"layout", "select-layout", "even-", "main-", "tiled",
		"rotate-window", NULL
	}},
	{ KEYS_CAT_MOUSE, {
		"Mouse", "mouse", "DoubleClick", "TripleClick", "Drag",
		"WheelUp", "WheelDown", NULL
	}}
};

/* Get category definition by type */
const struct keys_category_def *
keys_category_get(enum keys_category cat)
{
	u_int i;

	for (i = 0; i < nitems(categories); i++) {
		if (categories[i].type == cat)
			return (&categories[i]);
	}
	return (NULL);
}

/* Get category by index */
const struct keys_category_def *
keys_category_get_by_index(u_int idx)
{
	if (idx >= nitems(categories))
		return (NULL);
	return (&categories[idx]);
}

/* Get total number of categories */
u_int
keys_category_count(void)
{
	return (nitems(categories));
}

/* Match command/note to a category */
enum keys_category
keys_category_match(const char *command, const char *note)
{
	u_int i, j;
	const char *keyword;

	if (command == NULL && note == NULL)
		return (KEYS_CAT_MISC);

	/* Try to match command first */
	if (command != NULL && *command != '\0') {
		for (i = 0; i < nitems(category_keywords); i++) {
			for (j = 0; j < 10; j++) {
				keyword = category_keywords[i].keywords[j];
				if (keyword == NULL)
					break;
				if (keyword[0] != '\0' && strstr(command, keyword) != NULL)
					return (category_keywords[i].cat);
			}
		}
	}

	/* Try to match note if command didn't match */
	if (note != NULL && *note != '\0') {
		for (i = 0; i < nitems(category_keywords); i++) {
			for (j = 0; j < 10; j++) {
				keyword = category_keywords[i].keywords[j];
				if (keyword == NULL)
					break;
				if (keyword[0] != '\0' && strcasestr(note, keyword) != NULL)
					return (category_keywords[i].cat);
			}
		}
	}

	/* Default to miscellaneous */
	return (KEYS_CAT_MISC);
}

/* Compare two categories for sorting */
int
keys_category_cmp(enum keys_category a, enum keys_category b)
{
	/* Sort in the order defined in categories array */
	return ((int)a - (int)b);
}

/* Get category name for display */
const char *
keys_category_name(enum keys_category cat)
{
	const struct keys_category_def *def;

	def = keys_category_get(cat);
	if (def == NULL)
		return ("Unknown");
	return (def->name);
}

/* Get category icon */
const char *
keys_category_icon(enum keys_category cat)
{
	const struct keys_category_def *def;

	def = keys_category_get(cat);
	if (def == NULL)
		return ("");
	return (def->icon);
}

/* Get category color */
const char *
keys_category_color(enum keys_category cat)
{
	const struct keys_category_def *def;

	def = keys_category_get(cat);
	if (def == NULL)
		return ("#ABB2BF");
	return (def->color);
}

/* Get category description */
const char *
keys_category_description(enum keys_category cat)
{
	const struct keys_category_def *def;

	def = keys_category_get(cat);
	if (def == NULL)
		return ("");
	return (def->description);
}

/* Helper structure for grouping keys by category */
struct category_keys {
	enum keys_category	 cat;
	char			**keys;
	u_int			 count;
	u_int			 size;
};

/* Generate markdown cheatsheet of all keybindings */
char *
keys_generate_markdown_cheatsheet(void)
{
	struct category_keys	*catkeys;
	struct key_table	*table;
	struct key_binding	*bd;
	const struct keys_category_def *def;
	char			*markdown, *tmp, *cp, *line;
	enum keys_category	 cat;
	u_int			 i, j, ncat;
	size_t			 len, size;

	/* Initialize category array */
	ncat = keys_category_count();
	catkeys = xcalloc(ncat, sizeof(*catkeys));
	for (i = 0; i < ncat; i++) {
		catkeys[i].cat = i;
		catkeys[i].keys = NULL;
		catkeys[i].count = 0;
		catkeys[i].size = 0;
	}

	/* Collect all keybindings grouped by category */
	table = key_bindings_first_table();
	while (table != NULL) {
		bd = key_bindings_first(table);
		while (bd != NULL) {
			/* Skip mouse bindings and bindings without notes */
			if (KEYC_IS_MOUSE(bd->key) ||
			    bd->note == NULL ||
			    *bd->note == '\0') {
				bd = key_bindings_next(table, bd);
				continue;
			}

			/* Get command and classify */
			cp = cmd_list_print(bd->cmdlist, 0);
			if (cp == NULL) {
				bd = key_bindings_next(table, bd);
				continue;
			}
			cat = keys_category_match(cp, bd->note);
			free(cp);

			/* Add to category list */
			if (catkeys[cat].count >= catkeys[cat].size) {
				catkeys[cat].size = catkeys[cat].size == 0 ? 8 :
				    catkeys[cat].size * 2;
				catkeys[cat].keys = xreallocarray(
				    catkeys[cat].keys, catkeys[cat].size,
				    sizeof(*catkeys[cat].keys));
			}

			/* Format: "  - `key` - note" */
			xasprintf(&line, "  - `%s` - %s",
			    key_string_lookup_key(bd->key, 0), bd->note);
			catkeys[cat].keys[catkeys[cat].count++] = line;

			bd = key_bindings_next(table, bd);
		}
		table = key_bindings_next_table(table);
	}

	/* Build markdown string */
	markdown = xstrdup("# Smux Keybinding Cheatsheet\n\n");
	markdown = xstrdup(markdown);

	for (i = 0; i < ncat; i++) {
		if (catkeys[i].count == 0)
			continue;

		def = keys_category_get_by_index(i);
		if (def == NULL)
			continue;

		/* Add category header with icon */
		xasprintf(&tmp, "%s## %s %s\n\n%s\n\n",
		    markdown, def->icon, def->name, def->description);
		free(markdown);
		markdown = tmp;

		/* Add all keys in this category */
		for (j = 0; j < catkeys[i].count; j++) {
			len = strlen(markdown);
			size = len + strlen(catkeys[i].keys[j]) + 2;
			markdown = xrealloc(markdown, size);
			strlcat(markdown, catkeys[i].keys[j], size);
			strlcat(markdown, "\n", size);
		}

		/* Add blank line after category */
		len = strlen(markdown);
		size = len + 2;
		markdown = xrealloc(markdown, size);
		strlcat(markdown, "\n", size);
	}

	/* Cleanup */
	for (i = 0; i < ncat; i++) {
		for (j = 0; j < catkeys[i].count; j++)
			free(catkeys[i].keys[j]);
		free(catkeys[i].keys);
	}
	free(catkeys);

	return (markdown);
}
