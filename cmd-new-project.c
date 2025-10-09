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
 * Create a new project.
 */

static enum cmd_retval	cmd_new_project_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_new_project_entry = {
	.name = "new-project",
	.alias = "newp",

	.args = { "n:c:", 0, 0, NULL },
	.usage = "[-n project-name] [-c start-directory]",

	.flags = CMD_STARTSERVER,
	.exec = cmd_new_project_exec
};

static enum cmd_retval
cmd_new_project_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = cmd_get_args(self);
	struct client		*c = cmdq_get_client(item);
	const char		*pname, *cwd;
	struct project		*p;
	struct session		*s;
	struct environ		*env;
	struct options		*oo;
	struct termios		 tio, *tiop;
	struct spawn_context	 sc;
	char			*cause;

	/* Get project name from -n flag or generate with "project" prefix. */
	pname = args_get(args, 'n');

	/* Get working directory from -c flag or use "/" as fallback. */
	cwd = args_get(args, 'c');
	if (cwd == NULL)
		cwd = "/";

	/* Check if project already exists (only if name specified). */
	if (pname != NULL) {
		p = project_find(pname);
		if (p != NULL) {
			cmdq_error(item, "duplicate project: %s", pname);
			return (CMD_RETURN_ERROR);
		}
	}

	/* Create the project. */
	p = project_create("project", pname, cwd, NULL);
	if (p == NULL) {
		cmdq_error(item, "failed to create project");
		return (CMD_RETURN_ERROR);
	}

	/* Notify control mode clients. */
	/* notify_project("project-created", p); */  /* TODO: implement in task 18 */

	/* Print success message. */
	cmdq_print(item, "created project %s", p->name);

	/* Create initial session in the project to keep server alive. */
	env = environ_create();
	oo = options_create(global_s_options);

	/* Get terminal settings if available. */
	if (c != NULL && c->fd != -1) {
		if (tcgetattr(c->fd, &tio) == 0)
			tiop = &tio;
		else
			tiop = NULL;
	} else
		tiop = NULL;

	/* Create session with same name as project. */
	s = session_create("session", p->name, cwd, env, oo, tiop, p);
	if (s == NULL) {
		cmdq_error(item, "failed to create session in project");
		project_destroy(p, 1, __func__);
		return (CMD_RETURN_ERROR);
	}

	/* Set current session for the project. */
	p->curs = s;

	/* Spawn the initial window. */
	memset(&sc, 0, sizeof sc);
	sc.item = item;
	sc.s = s;
	sc.name = NULL;
	sc.argc = 0;
	sc.argv = NULL;
	sc.idx = -1;
	sc.cwd = cwd;
	sc.flags = 0;

	if (spawn_window(&sc, &cause) == NULL) {
		session_destroy(s, 0, __func__);
		project_destroy(p, 1, __func__);
		cmdq_error(item, "create window failed: %s", cause);
		free(cause);
		return (CMD_RETURN_ERROR);
	}

	return (CMD_RETURN_NORMAL);
}
