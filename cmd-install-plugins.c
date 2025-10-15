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
 * Install plugins declared in configuration.
 */

static enum cmd_retval	cmd_install_plugins_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_install_plugins_entry = {
	.name = "install-plugins",
	.alias = "plugin-install",

	.args = { "vf", 0, 0, NULL },
	.usage = "[-vf]",

	.flags = CMD_STARTSERVER,
	.exec = cmd_install_plugins_exec
};

static enum cmd_retval
cmd_install_plugins_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = cmd_get_args(self);
	int			 verbose = args_has(args, 'v');
	int			 force = args_has(args, 'f');
	struct plugin		*p;
	int			 total = 0, installed = 0, failed = 0;

	/* Initialize plugin directories */
	plugin_init_directories();

	if (verbose)
		cmdq_print(item, "Installing plugins...");

	/* Install all parsed plugins */
	RB_FOREACH(p, plugins, &plugins) {
		if (p->status != PLUGIN_STATUS_PARSED && !force)
			continue;

		total++;

		if (verbose)
			cmdq_print(item, "Installing plugin: %s from %s", p->name, p->source);

		if (plugin_install(p) == 0) {
			installed++;
			if (verbose)
				cmdq_print(item, "✓ Plugin %s installed successfully", p->name);
		} else {
			failed++;
			cmdq_error(item, "✗ Failed to install plugin %s: %s",
				  p->name, p->last_error ? p->last_error : "unknown error");
		}
	}

	if (total == 0) {
		cmdq_print(item, "No plugins to install. Add plugins with: set -g @plugin 'plugin-name'");
		return (CMD_RETURN_NORMAL);
	}

	cmdq_print(item, "Plugin installation complete: %d installed, %d failed, %d total",
		  installed, failed, total);

	/* Source installed plugins */
	if (installed > 0) {
		if (verbose)
			cmdq_print(item, "Loading plugin configurations...");
		plugin_source_all();
	}

	return (CMD_RETURN_NORMAL);
}