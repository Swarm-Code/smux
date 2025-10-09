/* $OpenBSD$ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicholas.marriott@gmail.com>
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

#include "tmux.h"

/*
 * Destroy project, detaching all sessions from it without destroying them.
 *
 * Note this deliberately has no alias to make it hard to hit by accident.
 */

static enum cmd_retval	cmd_kill_project_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_kill_project_entry = {
	.name = "kill-project",
	.alias = NULL,

	.args = { "at:", 0, 0, NULL },
	.usage = "[-a] [-t target-project]",

	/* TODO: Add target resolution when CMD_FIND_PROJECT is implemented (task 19) */
	/* .target = { 't', CMD_FIND_PROJECT, 0 }, */

	.flags = 0,
	.exec = cmd_kill_project_exec
};

static enum cmd_retval
cmd_kill_project_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = cmd_get_args(self);
	struct project		*p, *ploop, *ptmp;
	const char		*target;

	/* Get target project. */
	target = args_get(args, 't');
	if (target == NULL) {
		/* No target specified - try to use current session's project */
		struct client *c = cmdq_get_client(item);
		if (c != NULL && c->session != NULL && c->session->project != NULL) {
			p = c->session->project;
		} else {
			cmdq_error(item, "no current project");
			return (CMD_RETURN_ERROR);
		}
	} else {
		/* Find target project by name (or #ID when task 19 adds project_find_by_id_str) */
		p = project_find(target);
		if (p == NULL) {
			cmdq_error(item, "project not found: %s", target);
			return (CMD_RETURN_ERROR);
		}
	}

	/* Check for -a flag (kill all OTHER projects). */
	if (args_has(args, 'a')) {
		RB_FOREACH_SAFE(ploop, projects, &projects, ptmp) {
			if (ploop != p)
				project_destroy(ploop, 1, __func__);
		}
	} else {
		/* Kill just the target project. */
		project_destroy(p, 1, __func__);
	}

	return (CMD_RETURN_NORMAL);
}
