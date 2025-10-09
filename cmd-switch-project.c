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

#include "tmux.h"

/*
 * Switch client to a session in a project.
 */

static enum cmd_retval	cmd_switch_project_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_switch_project_entry = {
	.name = "switch-project",
	.alias = "switchp",

	.args = { "t:", 0, 0, NULL },
	.usage = "[-t target-project]",

	/* TODO: Add target resolution when CMD_FIND_PROJECT is implemented (task 19) */
	/* .target = { 't', CMD_FIND_PROJECT, 0 }, */

	.flags = 0,
	.exec = cmd_switch_project_exec
};

static enum cmd_retval
cmd_switch_project_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = cmd_get_args(self);
	struct client		*c;
	struct project		*p;
	struct session		*s;
	const char		*target;

	/* Get client. */
	c = cmdq_get_client(item);
	if (c == NULL) {
		cmdq_error(item, "no client");
		return (CMD_RETURN_ERROR);
	}

	/* Get target project. */
	target = args_get(args, 't');
	if (target == NULL) {
		/* No target - use current session's project */
		if (c->session != NULL && c->session->project != NULL) {
			p = c->session->project;
		} else {
			cmdq_error(item, "no current project");
			return (CMD_RETURN_ERROR);
		}
	} else {
		p = project_find(target);
		if (p == NULL) {
			cmdq_error(item, "project not found: %s", target);
			return (CMD_RETURN_ERROR);
		}
	}

	/* Find a session to switch to in the project. */
	s = p->curs;  /* Try current session first */
	if (s == NULL) {
		/* No current session - get first session in project */
		s = RB_MIN(sessions, &p->sessions);
		if (s == NULL) {
			cmdq_error(item, "project has no sessions: %s", p->name);
			return (CMD_RETURN_ERROR);
		}
	}

	/* Switch client to the session. */
	if (c->session != NULL && c->session != s)
		c->last_session = c->session;
	c->session = s;

	/* Update project's current session. */
	p->curs = s;

	/* Notify that client changed session. */
	notify_client("client-session-changed", c);

	/* Redraw client. */
	server_redraw_client(c);

	return (CMD_RETURN_NORMAL);
}
