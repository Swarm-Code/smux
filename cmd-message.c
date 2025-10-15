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

#include <stdlib.h>
#include <string.h>

#include "tmux.h"

/*
 * Send a message to a pane (text + Enter key).
 * Uses intelligent pane stability detection and proper timer-based delays
 * to ensure text is sent character-by-character like send-keys, then Enter
 * is sent separately after text is fully processed.
 */

struct cmd_message_data {
	struct session		*s;
	int			 wl_idx;
	int			 wp_id;
	char			*text;
	char			*last_content;
	int			 stable_checks;
	int			 max_attempts;
	struct event		 timer;
	enum {
		WAITING_STABLE,
		SENDING_TEXT,
		SENDING_ENTER
	} state;
};

static enum cmd_retval		cmd_message_exec(struct cmd *, struct cmdq_item *);
static void			cmd_message_timer(int, short, void *);
static void			cmd_message_free(void *);

const struct cmd_entry cmd_message_entry = {
	.name = "message",
	.alias = "msg",

	.args = { "t:", 1, 1, NULL },
	.usage = "[-t target-pane] text",

	.target = { 't', CMD_FIND_PANE, 0 },

	.flags = CMD_AFTERHOOK,
	.exec = cmd_message_exec
};

static void
cmd_message_timer(__unused int fd, __unused short events, void *arg)
{
	struct cmd_message_data	*data = arg;
	struct session			*s;
	struct winlink			*wl;
	struct window_pane		*wp;
	struct grid			*gd;
	char				*content;
	struct timeval			 tv;
	u_int				 start_line;

	/* Find the target pane */
	s = session_find_by_id(data->s->id);
	if (s == NULL)
		goto out;

	wl = winlink_find_by_index(&s->windows, data->wl_idx);
	if (wl == NULL)
		goto out;

	wp = window_pane_find_by_id(data->wp_id);
	if (wp == NULL)
		goto out;

	gd = wp->base.grid;

	switch (data->state) {
	case WAITING_STABLE:
		/* Check if pane content has stabilized */
		if (data->max_attempts-- <= 0) {
			/* Timeout after 30 seconds, send anyway */
			data->state = SENDING_TEXT;
			tv.tv_sec = 0;
			tv.tv_usec = 0;
			evtimer_add(&data->timer, &tv);
			return;
		}

		/* Get last line of pane content (cursor line) for stability check */
		start_line = gd->hsize + gd->sy - 1;

		content = grid_string_cells(gd, 0, start_line,
		    gd->sx, NULL, 0, NULL);

		if (data->last_content != NULL) {
			/* Compare with previous snapshot */
			if (strcmp(content, data->last_content) == 0) {
				/* Content unchanged - pane is stable */
				data->stable_checks++;
				if (data->stable_checks >= 2) {
					/* Stable for 2 checks (400ms), send text */
					free(content);
					free(data->last_content);
					data->last_content = NULL;
					data->state = SENDING_TEXT;
					tv.tv_sec = 0;
					tv.tv_usec = 0;
					evtimer_add(&data->timer, &tv);
					return;
				}
			} else {
				/* Content changed, reset stability counter */
				data->stable_checks = 0;
			}
			free(data->last_content);
		}

		data->last_content = content;

		/* Check again in 200ms */
		tv.tv_sec = 0;
		tv.tv_usec = 200000;
		evtimer_add(&data->timer, &tv);
		break;

	case SENDING_TEXT:
		/* Send the text character by character like send-keys does */
		{
			struct utf8_data	*ud, *loop;
			utf8_char		 uc;
			key_code		 key;

			ud = utf8_fromcstr(data->text);
			for (loop = ud; loop->size != 0; loop++) {
				if (loop->size == 1 && loop->data[0] <= 0x7f)
					key = loop->data[0];
				else {
					if (utf8_from_data(loop, &uc) != UTF8_DONE)
						continue;
					key = uc;
				}
				window_pane_key(wp, NULL, s, wl, key, NULL);
			}
			free(ud);
		}

		/* Schedule Enter after 500ms */
		data->state = SENDING_ENTER;
		tv.tv_sec = 0;
		tv.tv_usec = 500000;
		evtimer_add(&data->timer, &tv);
		break;

	case SENDING_ENTER:
		/* Send Enter key */
		window_pane_key(wp, NULL, s, wl, C0_CR, NULL);
		goto out;
	}

	return;

out:
	cmd_message_free(data);
}

static void
cmd_message_free(void *arg)
{
	struct cmd_message_data	*data = arg;

	evtimer_del(&data->timer);
	free(data->text);
	free(data->last_content);
	free(data);
}

static enum cmd_retval
cmd_message_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args			*args = cmd_get_args(self);
	struct cmd_find_state		*target = cmdq_get_target(item);
	struct cmd_message_data	*data;
	struct timeval			 tv = { .tv_sec = 0, .tv_usec = 200000 }; /* Start checking in 200ms */
	const char			*text;

	text = args_string(args, 0);
	if (text == NULL || *text == '\0')
		return (CMD_RETURN_ERROR);

	/* Set up state machine to wait for pane stability */
	data = xcalloc(1, sizeof *data);
	data->s = target->s;
	data->wl_idx = target->wl->idx;
	data->wp_id = target->wp->id;
	data->text = xstrdup(text);
	data->last_content = NULL;
	data->stable_checks = 0;
	data->max_attempts = 150; /* 150 * 200ms = 30 seconds max wait */
	data->state = WAITING_STABLE;

	evtimer_set(&data->timer, cmd_message_timer, data);
	evtimer_add(&data->timer, &tv);

	return (CMD_RETURN_NORMAL);
}
