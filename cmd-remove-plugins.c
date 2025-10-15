/* $OpenBSD$ */

/*
 * Copyright (c) 2025 Swarm Project Contributors
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

#include <string.h>

#include "tmux.h"

/*
 * Remove plugins not declared in configuration.
 */

static enum cmd_retval	cmd_remove_plugins_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_remove_plugins_entry = {
	.name = "remove-plugins",
	.alias = "plugin-remove",

	.args = { "vf", 0, 1, NULL },
	.usage = "[-vf] [plugin-name]",

	.flags = CMD_STARTSERVER,
	.exec = cmd_remove_plugins_exec
};

static enum cmd_retval
cmd_remove_plugins_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = cmd_get_args(self);
	int			 verbose = args_has(args, 'v');
	int			 force = args_has(args, 'f');
	const char		*plugin_name = NULL;
	struct plugin		*p, *tmp;
	int			 total = 0, removed = 0;

	if (args_count(args) > 0)
		plugin_name = args_string(args, 0);

	if (verbose) {
		if (plugin_name)
			cmdq_print(item, "Removing plugin: %s", plugin_name);
		else
			cmdq_print(item, "Removing orphaned plugins...");
	}

	/* Remove specific plugin or orphaned plugins */
	RB_FOREACH_SAFE(p, plugins, &plugins, tmp) {
		int should_remove = 0;

		if (plugin_name != NULL) {
			/* Remove specific plugin */
			if (strcmp(p->name, plugin_name) == 0)
				should_remove = 1;
		} else {
			/* Remove plugins that are no longer parsed (orphaned) */
			if (p->status != PLUGIN_STATUS_PARSED || force)
				should_remove = 1;
		}

		if (!should_remove)
			continue;

		total++;

		if (verbose)
			cmdq_print(item, "Removing plugin: %s", p->name);

		if (plugin_remove(p) == 0) {
			removed++;
			if (verbose)
				cmdq_print(item, "✓ Plugin %s removed successfully", p->name);
		} else {
			cmdq_error(item, "✗ Failed to remove plugin %s", p->name);
		}
	}

	if (total == 0) {
		if (plugin_name)
			cmdq_print(item, "Plugin '%s' not found", plugin_name);
		else
			cmdq_print(item, "No orphaned plugins to remove");
		return (CMD_RETURN_NORMAL);
	}

	cmdq_print(item, "Plugin removal complete: %d removed, %d total",
		  removed, total);

	return (CMD_RETURN_NORMAL);
}