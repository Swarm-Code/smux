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
#include <sys/time.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "tmux.h"

struct projects      projects;
u_int               next_project_id;

static void project_free(int, short, void *);

int
project_cmp(struct project *p1, struct project *p2)
{
	return (strcmp(p1->name, p2->name));
}
RB_GENERATE(projects, project, entry, project_cmp);

/*
 * Find if project is still alive. This is true if it is still on the global
 * projects list.
 */
int
project_alive(struct project *p)
{
	struct project *p_loop;

	RB_FOREACH(p_loop, projects, &projects) {
		if (p_loop == p)
			return (1);
	}
	return (0);
}

/* Find project by name. */
struct project *
project_find(const char *name)
{
	struct project	p;

	p.name = (char *) name;
	return (RB_FIND(projects, &projects, &p));
}

/* Find project by id parsed from a string. */
struct project *
project_find_by_id_str(const char *s)
{
	const char	*errstr;
	u_int		 id;

	if (*s != '#')
		return (NULL);

	id = strtonum(s + 1, 0, UINT_MAX, &errstr);
	if (errstr != NULL)
		return (NULL);
	return (project_find_by_id(id));
}

/* Find project by id. */
struct project *
project_find_by_id(u_int id)
{
	struct project	*p;

	RB_FOREACH(p, projects, &projects) {
		if (p->id == id)
			return (p);
	}
	return (NULL);
}

/* Create a new project. */
struct project *
project_create(const char *prefix, const char *name, const char *cwd,
    struct environ *env)
{
	struct project  *p;

	p = xcalloc(1, sizeof *p);
	p->references = 1;
	p->flags = 0;

	p->cwd = xstrdup(cwd);

	RB_INIT(&p->sessions);
	p->curs = NULL;

	p->environ = env;

	if (name != NULL) {
		p->name = xstrdup(name);
		p->id = next_project_id++;
	} else {
		do {
			p->id = next_project_id++;
			free(p->name);
			if (prefix != NULL)
				xasprintf(&p->name, "%s-%u", prefix, p->id);
			else
				xasprintf(&p->name, "%u", p->id);
		} while (RB_FIND(projects, &projects, p) != NULL);
	}

	if (p->environ == NULL)
		p->environ = environ_create();

	if (gettimeofday(&p->creation_time, NULL) != 0)
		fatal("gettimeofday failed");
	p->activity_time = p->creation_time;

	RB_INSERT(projects, &projects, p);

	log_debug("new project %s $%u", p->name, p->id);

	return (p);
}

/* Destroy a project. */
void
project_destroy(struct project *p, int notify, const char *from)
{
	struct session	*s, *stmp;

	log_debug("%s: project %s (from %s)", __func__, p->name, from);

	/* Check if already destroyed. */
	if (p->curs == NULL)
		return;

	/* Mark as destroyed. */
	p->curs = NULL;

	/* Remove from global tree FIRST. */
	RB_REMOVE(projects, &projects, p);

	/* Send notification if requested. */
	/* TODO: Implement notify_project() function */
	/* if (notify)
		notify_project("project-closed", p); */
	(void)notify;

	/* Detach all sessions from this project (but don't destroy them). */
	RB_FOREACH_SAFE(s, sessions, &p->sessions, stmp) {
		s->project = NULL;
		RB_REMOVE(sessions, &p->sessions, s);
	}

	/* Free environment. */
	if (p->environ != NULL) {
		environ_free(p->environ);
		p->environ = NULL;
	}

	/* Free working directory. */
	free((void *)p->cwd);
	p->cwd = NULL;

	/* Deferred cleanup via reference counting. */
	project_remove_ref(p, __func__);
}

void
project_add_ref(struct project *p, const char *from)
{
	p->references++;
	log_debug("%s: %s %s %d", __func__, p->name, from, p->references);
}

void
project_remove_ref(struct project *p, const char *from)
{
	p->references--;
	log_debug("%s: %s %s %d", __func__, p->name, from, p->references);
	if (p->references == 0)
		event_once(-1, EV_TIMEOUT, project_free, p, NULL);
}

static void
project_free(int fd, short events, void *arg)
{
	struct project  *p = arg;

	(void)fd;
	(void)events;

	log_debug("%s: project %s (references %d)", __func__, p->name,
	    p->references);

	if (p->references == 0) {
		if (p->environ != NULL)
			environ_free(p->environ);
		free(p->name);
		free(p);
	}
}

/* Attach a session to a project. */
struct session *
project_attach_session(struct project *p, struct session *s)
{
	s->project = p;
	RB_INSERT(sessions, &p->sessions, s);

	/* TODO: Implement notify_project() function */
	/* notify_project("session-linked", p); */

	if (p->curs == NULL)
		p->curs = s;

	return (s);
}

/* Detach a session from a project. */
int
project_detach_session(struct project *p, struct session *s)
{
	s->project = NULL;
	RB_REMOVE(sessions, &p->sessions, s);

	/* TODO: Implement notify_project() function */
	/* notify_project("session-unlinked", p); */

	if (p->curs == s) {
		p->curs = RB_MIN(sessions, &p->sessions);
		if (p->curs == NULL)
			return (1);  /* Project now empty */
	}

	return (0);
}
