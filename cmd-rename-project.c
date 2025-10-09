/* $OpenBSD$ */

/*
 * Copyright (c) 2025 Smux Project
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

#include <stdlib.h>
#include <string.h>

#include "tmux.h"

/*
 * Change project name.
 */

static enum cmd_retval	cmd_rename_project_exec(struct cmd *,
			    struct cmdq_item *);

const struct cmd_entry cmd_rename_project_entry = {
	.name = "rename-project",
	.alias = "renamep",

	.args = { "t:", 1, 1, NULL },
	.usage = "[-t target-project] new-name",

	/* TODO: Add target resolution when CMD_FIND_PROJECT is implemented (task 19) */
	/* .target = { 't', CMD_FIND_PROJECT, 0 }, */

	.flags = CMD_AFTERHOOK,
	.exec = cmd_rename_project_exec
};

static enum cmd_retval
cmd_rename_project_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = cmd_get_args(self);
	struct project		*p, *pfind;
	const char		*target, *newname;

	/* Get target project. */
	target = args_get(args, 't');
	if (target == NULL) {
		/* No target - use current session's project */
		struct client *c = cmdq_get_client(item);
		if (c != NULL && c->session != NULL && c->session->project != NULL) {
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

	/* Get new name from positional argument. */
	newname = args_string(args, 0);
	if (newname == NULL || *newname == '\0') {
		cmdq_error(item, "invalid project name");
		return (CMD_RETURN_ERROR);
	}

	/* Check if new name is same as current name. */
	if (strcmp(newname, p->name) == 0)
		return (CMD_RETURN_NORMAL);  /* No change needed */

	/* Check if new name already exists. */
	pfind = project_find(newname);
	if (pfind != NULL) {
		cmdq_error(item, "duplicate project: %s", newname);
		return (CMD_RETURN_ERROR);
	}

	/*
	 * CRITICAL: Remove from tree, rename, re-insert.
	 * The projects RB tree is indexed by name (project_cmp compares
	 * p1->name vs p2->name). Changing p->name while the node is in
	 * the tree breaks tree invariants. We MUST remove the node first,
	 * change the name, then re-insert with the new name as the key.
	 */
	RB_REMOVE(projects, &projects, p);
	free(p->name);
	p->name = xstrdup(newname);
	RB_INSERT(projects, &projects, p);

	/* Notify control mode clients. */
	/* notify_project("project-renamed", p); */  /* TODO: implement in task 18 */

	return (CMD_RETURN_NORMAL);
}
