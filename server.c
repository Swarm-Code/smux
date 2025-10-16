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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "tmux.h"

/* Debug tracing */
#define DEBUG_TRACE_IMPL
int debug_trace_level = 5;
FILE *debug_trace_file = NULL;
#include "debug-trace.h"

/*
 * Main server functions.
 */

struct clients		 clients;

struct tmuxproc		*server_proc;
static int		 server_fd = -1;
static uint64_t		 server_client_flags;
static int		 server_exit;
static struct event	 server_ev_accept;
static struct event	 server_ev_tidy;

struct cmd_find_state	 marked_pane;

static u_int		 message_next;
struct message_list	 message_log;

time_t			 current_time;

static int	server_loop(void);
static void	server_send_exit(void);
static void	server_accept(int, short, void *);
static void	server_signal(int);
static void	server_child_signal(void);
static void	server_child_exited(pid_t, int);
static void	server_child_stopped(pid_t, int);

/* Set marked pane. */
void
server_set_marked(struct session *s, struct winlink *wl, struct window_pane *wp)
{
	cmd_find_clear_state(&marked_pane, 0);
	marked_pane.s = s;
	marked_pane.wl = wl;
	if (wl != NULL)
		marked_pane.w = wl->window;
	marked_pane.wp = wp;
}

/* Clear marked pane. */
void
server_clear_marked(void)
{
	cmd_find_clear_state(&marked_pane, 0);
}

/* Is this the marked pane? */
int
server_is_marked(struct session *s, struct winlink *wl, struct window_pane *wp)
{
	if (s == NULL || wl == NULL || wp == NULL)
		return (0);
	if (marked_pane.s != s || marked_pane.wl != wl)
		return (0);
	if (marked_pane.wp != wp)
		return (0);
	return (server_check_marked());
}

/* Check if the marked pane is still valid. */
int
server_check_marked(void)
{
	return (cmd_find_valid_state(&marked_pane));
}

/* Create server socket. */
int
server_create_socket(uint64_t flags, char **cause)
{
	struct sockaddr_un	sa;
	size_t			size;
	mode_t			mask;
	int			fd, saved_errno;

	memset(&sa, 0, sizeof sa);
	sa.sun_family = AF_UNIX;
	size = strlcpy(sa.sun_path, socket_path, sizeof sa.sun_path);
	if (size >= sizeof sa.sun_path) {
		errno = ENAMETOOLONG;
		goto fail;
	}
	unlink(sa.sun_path);

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		goto fail;

	if (flags & CLIENT_DEFAULTSOCKET)
		mask = umask(S_IXUSR|S_IXGRP|S_IRWXO);
	else
		mask = umask(S_IXUSR|S_IRWXG|S_IRWXO);
	if (bind(fd, (struct sockaddr *)&sa, sizeof sa) == -1) {
		saved_errno = errno;
		close(fd);
		errno = saved_errno;
		goto fail;
	}
	umask(mask);

	if (listen(fd, 128) == -1) {
		saved_errno = errno;
		close(fd);
		errno = saved_errno;
		goto fail;
	}
	setblocking(fd, 0);

	return (fd);

fail:
	if (cause != NULL) {
		xasprintf(cause, "error creating %s (%s)", socket_path,
		    strerror(errno));
	}
	return (-1);
}

/* Tidy up every hour. */
static void
server_tidy_event(__unused int fd, __unused short events, __unused void *data)
{
    struct timeval	tv = { .tv_sec = 3600 };
    uint64_t		t = get_timer();

    format_tidy_jobs();

#ifdef HAVE_MALLOC_TRIM
    malloc_trim(0);
#endif

    log_debug("%s: took %llu milliseconds", __func__,
        (unsigned long long)(get_timer() - t));
    evtimer_add(&server_ev_tidy, &tv);
}

/* Fork new server. */
int
server_start(struct tmuxproc *client, uint64_t flags, struct event_base *base,
    int lockfd, char *lockfile)
{
	int		 fd;
	sigset_t	 set, oldset;
	struct client	*c = NULL;
	char		*cause = NULL;
	struct timeval	 tv = { .tv_sec = 3600 };

	/* Initialize debug tracing */
	debug_trace_init("/tmp/smux-debug-trace.log");
	DEBUG_ENTER();
	DEBUG_VAR_PTR(client);
	DEBUG_INFO("flags=0x%lx, lockfd=%d", flags, lockfd);

	DEBUG_CHECKPOINT("signal_setup");
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, &oldset);


	DEBUG_CHECKPOINT("fork_daemon_check");
	if (~flags & CLIENT_NOFORK) {
		DEBUG_INFO("Forking daemon process");
		if (proc_fork_and_daemon(&fd) != 0) {
			sigprocmask(SIG_SETMASK, &oldset, NULL);
			DEBUG_INFO("Parent process returning fd=%d", fd);
			return (fd);
		}
		DEBUG_INFO("Child daemon process continuing");
	}
	DEBUG_CHECKPOINT("clear_signals");
	proc_clear_signals(client, 0);
	server_client_flags = flags;

	DEBUG_CHECKPOINT("event_reinit");
	if (event_reinit(base) != 0)
		fatalx("event_reinit failed");

	DEBUG_CHECKPOINT("proc_start");
	server_proc = proc_start("server");
	DEBUG_VAR_PTR(server_proc);

	DEBUG_CHECKPOINT("set_signals");
	proc_set_signals(server_proc, server_signal);
	sigprocmask(SIG_SETMASK, &oldset, NULL);

	DEBUG_CHECKPOINT("logging_setup");
	if (log_get_level() > 1)
		tty_create_log();
	if (pledge("stdio rpath wpath cpath fattr unix getpw recvfd proc exec "
	    "tty ps", NULL) != 0)
		fatal("pledge failed");

	DEBUG_CHECKPOINT("input_key_build");
	input_key_build();
	DEBUG_CHECKPOINT("utf8_update_width_cache");
	utf8_update_width_cache();
	DEBUG_CHECKPOINT("init_data_structures");
	RB_INIT(&windows);
	RB_INIT(&all_window_panes);
	TAILQ_INIT(&clients);
	RB_INIT(&sessions);
	RB_INIT(&projects);
	next_project_id = 0;
	DEBUG_CHECKPOINT("plugin_init_START");
	plugin_init();
	DEBUG_CHECKPOINT("plugin_init_END");

	DEBUG_CHECKPOINT("key_bindings_init_START");
	key_bindings_init();
	DEBUG_CHECKPOINT("key_bindings_init_END");

	DEBUG_CHECKPOINT("message_log_init");
	TAILQ_INIT(&message_log);
	gettimeofday(&start_time, NULL);

	DEBUG_CHECKPOINT("create_socket_START");
#ifdef HAVE_SYSTEMD
	server_fd = systemd_create_socket(flags, &cause);
#else
	server_fd = server_create_socket(flags, &cause);
#endif
	DEBUG_VAR_INT(server_fd);
	DEBUG_CHECKPOINT("create_socket_END");
	if (server_fd != -1)
		server_update_socket();
	if (~flags & CLIENT_NOFORK)
		c = server_client_create(fd);
	else
		options_set_number(global_options, "exit-empty", 0);

	if (lockfd >= 0) {
		DEBUG_INFO("Unlinking lockfile: %s", lockfile);
		unlink(lockfile);
		free(lockfile);
		close(lockfd);
	}

	if (cause != NULL) {
		if (c != NULL) {
			c->exit_message = cause;
			c->flags |= CLIENT_EXIT;
		} else {
			fprintf(stderr, "%s\n", cause);
			exit(1);
		}
	}

	DEBUG_CHECKPOINT("timer_setup");
	evtimer_set(&server_ev_tidy, server_tidy_event, NULL);
	evtimer_add(&server_ev_tidy, &tv);

	DEBUG_CHECKPOINT("acl_init_START");
	server_acl_init();
	DEBUG_CHECKPOINT("acl_init_END");

	DEBUG_CHECKPOINT("add_accept_START");
	server_add_accept(0);
	DEBUG_CHECKPOINT("add_accept_END");

	DEBUG_CHECKPOINT("proc_loop_START");
	DEBUG_INFO("Entering main event loop - THIS IS WHERE HANG MIGHT OCCUR");
	proc_loop(server_proc, server_loop);
	DEBUG_CHECKPOINT("proc_loop_END");

	DEBUG_INFO("Event loop exited normally");
	job_kill_all();
	status_prompt_save_history();

	debug_trace_close();
	exit(0);
}

/* Server loop callback. */
static int
server_loop(void)
{
	struct client	*c;
	u_int		 items;
	static u_int	 loop_count = 0;

	loop_count++;
	if (loop_count % 100 == 0) {
		DEBUG_HEARTBEAT();
		DEBUG_INFO("server_loop iteration %u", loop_count);
	}

	current_time = time(NULL);

	DEBUG_TRACE_MSG("cmdq_next processing start");
	do {
		items = cmdq_next(NULL);
		TAILQ_FOREACH(c, &clients, entry) {
			if (c->flags & CLIENT_IDENTIFIED)
				items += cmdq_next(c);
		}
	} while (items != 0);
	DEBUG_TRACE_MSG("cmdq_next processing end (items=%u)", items);

	DEBUG_TRACE_MSG("server_client_loop start");
	server_client_loop();
	DEBUG_TRACE_MSG("server_client_loop end");

	if (!options_get_number(global_options, "exit-empty") && !server_exit)
		return (0);

	if (!options_get_number(global_options, "exit-unattached")) {
		if (!RB_EMPTY(&sessions))
			return (0);
	}

	TAILQ_FOREACH(c, &clients, entry) {
		if (c->session != NULL)
			return (0);
	}

	/*
	 * No attached clients therefore want to exit - flush any waiting
	 * clients but don't actually exit until they've gone.
	 */
	cmd_wait_for_flush();
	if (!TAILQ_EMPTY(&clients))
		return (0);

	if (job_still_running())
		return (0);

	return (1);
}

/* Exit the server by killing all clients and windows. */
static void
server_send_exit(void)
{
	struct client	*c, *c1;
	struct session	*s, *s1;
	struct project	*p, *p1;

	cmd_wait_for_flush();

	TAILQ_FOREACH_SAFE(c, &clients, entry, c1) {
		if (c->flags & CLIENT_SUSPENDED)
			server_client_lost(c);
		else {
			c->flags |= CLIENT_EXIT;
			c->exit_type = CLIENT_EXIT_SHUTDOWN;
		}
		c->session = NULL;
	}

	RB_FOREACH_SAFE(s, sessions, &sessions, s1)
		session_destroy(s, 1, __func__);

	RB_FOREACH_SAFE(p, projects, &projects, p1)
		project_destroy(p, 0, __func__);
}

/* Update socket execute permissions based on whether sessions are attached. */
void
server_update_socket(void)
{
	struct session	*s;
	static int	 last = -1;
	int		 n, mode;
	struct stat      sb;

	n = 0;
	RB_FOREACH(s, sessions, &sessions) {
		if (s->attached != 0) {
			n++;
			break;
		}
	}

	if (n != last) {
		last = n;

		if (stat(socket_path, &sb) != 0)
			return;
		mode = sb.st_mode & ACCESSPERMS;
		if (n != 0) {
			if (mode & S_IRUSR)
				mode |= S_IXUSR;
			if (mode & S_IRGRP)
				mode |= S_IXGRP;
			if (mode & S_IROTH)
				mode |= S_IXOTH;
		} else
			mode &= ~(S_IXUSR|S_IXGRP|S_IXOTH);
		chmod(socket_path, mode);
	}
}

/* Callback for server socket. */
static void
server_accept(int fd, short events, __unused void *data)
{
	struct sockaddr_storage	 sa;
	socklen_t		 slen = sizeof sa;
	int			 newfd;
	struct client		*c;

	server_add_accept(0);
	if (!(events & EV_READ))
		return;

	newfd = accept(fd, (struct sockaddr *) &sa, &slen);
	if (newfd == -1) {
		if (errno == EAGAIN || errno == EINTR || errno == ECONNABORTED)
			return;
		if (errno == ENFILE || errno == EMFILE) {
			/* Delete and don't try again for 1 second. */
			server_add_accept(1);
			return;
		}
		fatal("accept failed");
	}

	if (server_exit) {
		close(newfd);
		return;
	}
	c = server_client_create(newfd);
	if (!server_acl_join(c)) {
		c->exit_message = xstrdup("access not allowed");
		c->flags |= CLIENT_EXIT;
	}
}

/*
 * Add accept event. If timeout is nonzero, add as a timeout instead of a read
 * event - used to backoff when running out of file descriptors.
 */
void
server_add_accept(int timeout)
{
	struct timeval tv = { timeout, 0 };

	if (server_fd == -1)
		return;

	if (event_initialized(&server_ev_accept))
		event_del(&server_ev_accept);

	if (timeout == 0) {
		event_set(&server_ev_accept, server_fd, EV_READ, server_accept,
		    NULL);
		event_add(&server_ev_accept, NULL);
	} else {
		event_set(&server_ev_accept, server_fd, EV_TIMEOUT,
		    server_accept, NULL);
		event_add(&server_ev_accept, &tv);
	}
}

/* Signal handler. */
static void
server_signal(int sig)
{
	int	fd;

	log_debug("%s: %s", __func__, strsignal(sig));
	switch (sig) {
	case SIGINT:
	case SIGTERM:
		server_exit = 1;
		server_send_exit();
		break;
	case SIGCHLD:
		server_child_signal();
		break;
	case SIGUSR1:
		event_del(&server_ev_accept);
		fd = server_create_socket(server_client_flags, NULL);
		if (fd != -1) {
			close(server_fd);
			server_fd = fd;
			server_update_socket();
		}
		server_add_accept(0);
		break;
	case SIGUSR2:
		proc_toggle_log(server_proc);
		break;
	}
}

/* Handle SIGCHLD. */
static void
server_child_signal(void)
{
	int	 status;
	pid_t	 pid;

	for (;;) {
		switch (pid = waitpid(WAIT_ANY, &status, WNOHANG|WUNTRACED)) {
		case -1:
			if (errno == ECHILD)
				return;
			fatal("waitpid failed");
		case 0:
			return;
		}
		if (WIFSTOPPED(status))
			server_child_stopped(pid, status);
		else if (WIFEXITED(status) || WIFSIGNALED(status))
			server_child_exited(pid, status);
	}
}

/* Handle exited children. */
static void
server_child_exited(pid_t pid, int status)
{
	struct window		*w, *w1;
	struct window_pane	*wp;

	RB_FOREACH_SAFE(w, windows, &windows, w1) {
		TAILQ_FOREACH(wp, &w->panes, entry) {
			if (wp->pid == pid) {
				wp->status = status;
				wp->flags |= PANE_STATUSREADY;

				log_debug("%%%u exited", wp->id);
				wp->flags |= PANE_EXITED;

				if (window_pane_destroy_ready(wp))
					server_destroy_pane(wp, 1);
				break;
			}
		}
	}
	job_check_died(pid, status);
}

/* Handle stopped children. */
static void
server_child_stopped(pid_t pid, int status)
{
	struct window		*w;
	struct window_pane	*wp;

	if (WSTOPSIG(status) == SIGTTIN || WSTOPSIG(status) == SIGTTOU)
		return;

	RB_FOREACH(w, windows, &windows) {
		TAILQ_FOREACH(wp, &w->panes, entry) {
			if (wp->pid == pid) {
				if (killpg(pid, SIGCONT) != 0)
					kill(pid, SIGCONT);
			}
		}
	}
	job_check_died(pid, status);
}

/* Add to message log. */
void
server_add_message(const char *fmt, ...)
{
	struct message_entry	*msg, *msg1;
	char			*s;
	va_list			 ap;
	u_int			 limit;

	va_start(ap, fmt);
	xvasprintf(&s, fmt, ap);
	va_end(ap);

	log_debug("message: %s", s);

	msg = xcalloc(1, sizeof *msg);
	gettimeofday(&msg->msg_time, NULL);
	msg->msg_num = message_next++;
	msg->msg = s;
	TAILQ_INSERT_TAIL(&message_log, msg, entry);

	limit = options_get_number(global_options, "message-limit");
	TAILQ_FOREACH_SAFE(msg, &message_log, entry, msg1) {
		if (msg->msg_num + limit >= message_next)
			break;
		free(msg->msg);
		TAILQ_REMOVE(&message_log, msg, entry);
		free(msg);
	}
}
