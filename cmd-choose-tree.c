/* $OpenBSD$ */

/*
 * Copyright (c) 2012 Thomas Adam <thomas@xteddy.org>
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
 * Enter a mode.
 */

static enum args_parse_type	cmd_choose_tree_args_parse(struct args *args,
				    u_int idx, char **cause);
static enum cmd_retval		cmd_choose_tree_exec(struct cmd *,
    				    struct cmdq_item *);

const struct cmd_entry cmd_choose_tree_entry = {
	.name = "choose-tree",
	.alias = NULL,

	.args = { "F:f:GK:NO:rst:wyZ", 0, 1, cmd_choose_tree_args_parse },
	.usage = "[-GNrswZ] [-F format] [-f filter] [-K key-format] "
		 "[-O sort-order] " CMD_TARGET_PANE_USAGE " [template]",

	.target = { 't', CMD_FIND_PANE, 0 },

	.flags = 0,
	.exec = cmd_choose_tree_exec
};

const struct cmd_entry cmd_choose_client_entry = {
	.name = "choose-client",
	.alias = NULL,

	.args = { "F:f:K:NO:rt:yZ", 0, 1, cmd_choose_tree_args_parse },
	.usage = "[-NrZ] [-F format] [-f filter] [-K key-format] "
		 "[-O sort-order] " CMD_TARGET_PANE_USAGE " [template]",

	.target = { 't', CMD_FIND_PANE, 0 },

	.flags = 0,
	.exec = cmd_choose_tree_exec
};

const struct cmd_entry cmd_choose_buffer_entry = {
	.name = "choose-buffer",
	.alias = NULL,

	.args = { "F:f:K:NO:rt:yZ", 0, 1, cmd_choose_tree_args_parse },
	.usage = "[-NrZ] [-F format] [-f filter] [-K key-format] "
		 "[-O sort-order] " CMD_TARGET_PANE_USAGE " [template]",

	.target = { 't', CMD_FIND_PANE, 0 },

	.flags = 0,
	.exec = cmd_choose_tree_exec
};

const struct cmd_entry cmd_customize_mode_entry = {
	.name = "customize-mode",
	.alias = NULL,

	.args = { "F:f:Nt:yZ", 0, 0, NULL },
	.usage = "[-NZ] [-F format] [-f filter] " CMD_TARGET_PANE_USAGE,

	.target = { 't', CMD_FIND_PANE, 0 },

	.flags = 0,
	.exec = cmd_choose_tree_exec
};

/*
 * Default templates for choose-tree display formats.
 * These define how projects, sessions, and windows appear in the tree.
 */

#define CHOOSE_TREE_PROJECT_TEMPLATE				\
	"#[fg=#95E6CB,bold]PROJECT 📂 #{project_name}#[fg=#BFBDB6,nobold] - #{project_sessions} sessions " \
	"#[fg=#565B66](#{t:project_created})#[default]"

#define CHOOSE_TREE_SESSION_TEMPLATE				\
	"#{?session_project,#[fg=#475266]  ├─ ,#[fg=#BFBDB6]}"	\
	"#[fg=#E6A95E,bold]SESSION #[fg=#BFBDB6,bold]🖥️  #{session_name}#[nobold] "	\
	"#{?session_project,,#[fg=#95E6CB][#{session_project}] }"	\
	"#[fg=#565B66]- #{session_windows} windows"		\
	"#{?session_grouped, "					\
		"#[fg=#565B66](group #{session_group}: "	\
		"#{session_group_list}),"			\
	"}"							\
	"#{?session_attached,#[fg=#95E6CB,bold] ● ACTIVE#[default],}"

#define CHOOSE_TREE_WINDOW_TEMPLATE				\
	"#{?window_marked_flag,#[reverse],}"			\
	"#[fg=#475266]    └─ #[fg=#D19A66,italics]PANE #[fg=#565B66,noitalics]🪟 "			\
	"#{?window_active,#[fg=#95E6CB,bold],#[fg=#BFBDB6]}"	\
	"#{window_name}#[nobold]"				\
	"#{?window_active,#[fg=#95E6CB] ●,#[fg=#565B66]}"	\
	"#{window_flags} "					\
	"#{?#{&&:#{==:#{window_panes},1},#{&&:#{pane_title},#{!=:#{pane_title},#{host_short}}}},#[fg=#565B66]: \"#{pane_title}\",}"	\
	"#[default]"

static enum args_parse_type
cmd_choose_tree_args_parse(__unused struct args *args, __unused u_int idx,
    __unused char **cause)
{
	return (ARGS_PARSE_COMMANDS_OR_STRING);
}

static enum cmd_retval
cmd_choose_tree_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args			*args = cmd_get_args(self);
	struct cmd_find_state		*target = cmdq_get_target(item);
	struct window_pane		*wp = target->wp;
	const struct window_mode	*mode;

	if (cmd_get_entry(self) == &cmd_choose_buffer_entry) {
		if (paste_is_empty())
			return (CMD_RETURN_NORMAL);
		mode = &window_buffer_mode;
	} else if (cmd_get_entry(self) == &cmd_choose_client_entry) {
		if (server_client_how_many() == 0)
			return (CMD_RETURN_NORMAL);
		mode = &window_client_mode;
	} else if (cmd_get_entry(self) == &cmd_customize_mode_entry)
		mode = &window_customize_mode;
	else
		mode = &window_tree_mode;

	window_pane_set_mode(wp, NULL, mode, target, args);
	return (CMD_RETURN_NORMAL);
}
