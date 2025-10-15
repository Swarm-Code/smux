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
#include <time.h>

#include "tmux.h"

/*
 * List plugins with status and information.
 */

static enum cmd_retval	cmd_list_plugins_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_list_plugins_entry = {
	.name = "list-plugins",
	.alias = "plugin-list",

	.args = { "v", 0, 0, NULL },
	.usage = "[-v]",

	.flags = CMD_STARTSERVER,
	.exec = cmd_list_plugins_exec
};

static const char *
plugin_status_string(enum plugin_status status)
{
	switch (status) {
	case PLUGIN_STATUS_NONE:
		return "none";
	case PLUGIN_STATUS_PARSED:
		return "parsed";
	case PLUGIN_STATUS_INSTALLING:
		return "installing";
	case PLUGIN_STATUS_INSTALLED:
		return "installed";
	case PLUGIN_STATUS_FAILED:
		return "failed";
	case PLUGIN_STATUS_UPDATING:
		return "updating";
	case PLUGIN_STATUS_REMOVING:
		return "removing";
	}
	return "unknown";
}

static enum cmd_retval
cmd_list_plugins_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = cmd_get_args(self);
	int			 verbose = args_has(args, 'v');
	struct plugin		*p;
	int			 count = 0;
	char			 time_str[64];
	struct tm		*tm;

	cmdq_print(item, "Plugins:");

	RB_FOREACH(p, plugins, &plugins) {
		count++;

		if (verbose) {
			/* Format installation time */
			if (p->install_time.tv_sec > 0) {
				tm = localtime(&p->install_time.tv_sec);
				strftime(time_str, sizeof time_str, "%Y-%m-%d %H:%M:%S", tm);
			} else {
				strlcpy(time_str, "never", sizeof time_str);
			}

			cmdq_print(item, "  %s:", p->name);
			cmdq_print(item, "    Source: %s%s%s", p->source,
				  p->branch ? "#" : "", p->branch ? p->branch : "");
			cmdq_print(item, "    Status: %s", plugin_status_string(p->status));
			cmdq_print(item, "    Scope: %s", p->is_global ? "global" :
				  (p->project ? p->project->name : "unknown"));
			cmdq_print(item, "    Path: %s", p->install_path);
			cmdq_print(item, "    Installed: %s", time_str);
			if (p->last_error)
				cmdq_print(item, "    Last Error: %s", p->last_error);
			cmdq_print(item, "");
		} else {
			/* Compact format */
			char status_char;
			switch (p->status) {
			case PLUGIN_STATUS_INSTALLED:
				status_char = '✓';
				break;
			case PLUGIN_STATUS_FAILED:
				status_char = '✗';
				break;
			case PLUGIN_STATUS_PARSED:
				status_char = '○';
				break;
			case PLUGIN_STATUS_INSTALLING:
			case PLUGIN_STATUS_UPDATING:
				status_char = '⚬';
				break;
			default:
				status_char = '?';
				break;
			}

			cmdq_print(item, "  %c %s (%s) - %s%s%s", status_char, p->name,
				  plugin_status_string(p->status), p->source,
				  p->branch ? "#" : "", p->branch ? p->branch : "");
		}
	}

	if (count == 0) {
		cmdq_print(item, "  No plugins found. Add plugins with: set -g @plugin 'plugin-name'");
	} else {
		cmdq_print(item, "");
		cmdq_print(item, "Total: %d plugin%s", count, count == 1 ? "" : "s");
		if (!verbose) {
			cmdq_print(item, "Use -v for detailed information");
		}
	}

	return (CMD_RETURN_NORMAL);
}