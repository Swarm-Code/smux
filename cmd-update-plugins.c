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
 * Update installed plugins.
 */

static enum cmd_retval	cmd_update_plugins_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_update_plugins_entry = {
	.name = "update-plugins",
	.alias = "plugin-update",

	.args = { "v", 0, 0, NULL },
	.usage = "[-v]",

	.flags = CMD_STARTSERVER,
	.exec = cmd_update_plugins_exec
};

static enum cmd_retval
cmd_update_plugins_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = cmd_get_args(self);
	int			 verbose = args_has(args, 'v');
	struct plugin		*p;
	int			 total = 0, updated = 0, failed = 0;

	if (verbose)
		cmdq_print(item, "Updating plugins...");

	/* Update all installed plugins */
	RB_FOREACH(p, plugins, &plugins) {
		if (p->status != PLUGIN_STATUS_INSTALLED)
			continue;

		total++;

		if (verbose)
			cmdq_print(item, "Updating plugin: %s", p->name);

		if (plugin_update(p) == 0) {
			updated++;
			if (verbose)
				cmdq_print(item, "✓ Plugin %s updated successfully", p->name);
		} else {
			failed++;
			cmdq_error(item, "✗ Failed to update plugin %s: %s",
				  p->name, p->last_error ? p->last_error : "unknown error");
		}
	}

	if (total == 0) {
		cmdq_print(item, "No plugins to update. Install plugins first with: install-plugins");
		return (CMD_RETURN_NORMAL);
	}

	cmdq_print(item, "Plugin update complete: %d updated, %d failed, %d total",
		  updated, failed, total);

	/* Reload plugin configurations */
	if (updated > 0) {
		if (verbose)
			cmdq_print(item, "Reloading plugin configurations...");
		plugin_source_all();
	}

	return (CMD_RETURN_NORMAL);
}