/* $OpenBSD$ */

/*
 * Copyright (c) 2008 Nicholas Marriott <nicholas.marriott@gmail.com>
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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tmux.h"

struct client		 *cfg_client;
int			  cfg_finished;
static char		**cfg_causes;
static u_int		  cfg_ncauses;
static struct cmdq_item	 *cfg_item;

int                       cfg_quiet = 1;
char                    **cfg_files;
u_int                     cfg_nfiles;

static enum cmd_retval
cfg_client_done(__unused struct cmdq_item *item, __unused void *data)
{
	if (!cfg_finished)
		return (CMD_RETURN_WAIT);
	return (CMD_RETURN_NORMAL);
}

static enum cmd_retval
cfg_done(__unused struct cmdq_item *item, __unused void *data)
{
	if (cfg_finished)
		return (CMD_RETURN_NORMAL);
	cfg_finished = 1;

	cfg_show_causes(NULL);

	if (cfg_item != NULL)
		cmdq_continue(cfg_item);

	status_prompt_load_history();

	return (CMD_RETURN_NORMAL);
}

void
start_cfg(void)
{
	struct client	 *c;
	u_int		  i;
	int		  flags = 0;

	/* Initialize plugin directories early in server startup */
	plugin_init_directories();

	/*
	 * Configuration files are loaded without a client, so commands are run
	 * in the global queue with item->client NULL.
	 *
	 * However, we must block the initial client (but just the initial
	 * client) so that its command runs after the configuration is loaded.
	 * Because start_cfg() is called so early, we can be sure the client's
	 * command queue is currently empty and our callback will be at the
	 * front - we need to get in before MSG_COMMAND.
	 */
	cfg_client = c = TAILQ_FIRST(&clients);
	if (c != NULL) {
		cfg_item = cmdq_get_callback(cfg_client_done, NULL);
		cmdq_append(c, cfg_item);
	}

	if (cfg_quiet)
		flags = CMD_PARSE_QUIET;
	for (i = 0; i < cfg_nfiles; i++)
		load_cfg(cfg_files[i], c, NULL, NULL, flags, NULL);

	cmdq_append(NULL, cmdq_get_callback(cfg_done, NULL));
}

int
load_cfg(const char *path, struct client *c, struct cmdq_item *item,
    struct cmd_find_state *current, int flags, struct cmdq_item **new_item)
{
	FILE			*f;
	struct cmd_parse_input	 pi;
	struct cmd_parse_result	*pr;
	struct cmdq_item	*new_item0;
	struct cmdq_state	*state;

	if (new_item != NULL)
		*new_item = NULL;

	log_debug("loading %s", path);
	if ((f = fopen(path, "rb")) == NULL) {
		if (errno == ENOENT && (flags & CMD_PARSE_QUIET))
			return (0);
		cfg_add_cause("%s: %s", path, strerror(errno));
		return (-1);
	}

	memset(&pi, 0, sizeof pi);
	pi.flags = flags;
	pi.file = path;
	pi.line = 1;
	pi.item = item;
	pi.c = c;

	pr = cmd_parse_from_file(f, &pi);
	fclose(f);
	if (pr->status == CMD_PARSE_ERROR) {
		cfg_add_cause("%s", pr->error);
		free(pr->error);
		return (-1);
	}
	if (flags & CMD_PARSE_PARSEONLY) {
		cmd_list_free(pr->cmdlist);
		return (0);
	}

	if (item != NULL)
		state = cmdq_copy_state(cmdq_get_state(item), current);
	else
		state = cmdq_new_state(NULL, NULL, 0);
	cmdq_add_format(state, "current_file", "%s", pi.file);

	new_item0 = cmdq_get_command(pr->cmdlist, state);
	if (item != NULL)
		new_item0 = cmdq_insert_after(item, new_item0);
	else
		new_item0 = cmdq_append(NULL, new_item0);
	cmd_list_free(pr->cmdlist);
	cmdq_free_state(state);

	if (new_item != NULL)
		*new_item = new_item0;
	return (0);
}

int
load_cfg_from_buffer(const void *buf, size_t len, const char *path,
    struct client *c, struct cmdq_item *item, struct cmd_find_state *current,
    int flags, struct cmdq_item **new_item)
{
	struct cmd_parse_input	 pi;
	struct cmd_parse_result	*pr;
	struct cmdq_item	*new_item0;
	struct cmdq_state	*state;

	if (new_item != NULL)
		*new_item = NULL;

	log_debug("loading %s", path);

	memset(&pi, 0, sizeof pi);
	pi.flags = flags;
	pi.file = path;
	pi.line = 1;
	pi.item = item;
	pi.c = c;

	pr = cmd_parse_from_buffer(buf, len, &pi);
	if (pr->status == CMD_PARSE_ERROR) {
		cfg_add_cause("%s", pr->error);
		free(pr->error);
		return (-1);
	}
	if (flags & CMD_PARSE_PARSEONLY) {
		cmd_list_free(pr->cmdlist);
		return (0);
	}

	if (item != NULL)
		state = cmdq_copy_state(cmdq_get_state(item), current);
	else
		state = cmdq_new_state(NULL, NULL, 0);
	cmdq_add_format(state, "current_file", "%s", pi.file);

	new_item0 = cmdq_get_command(pr->cmdlist, state);
	if (item != NULL)
		new_item0 = cmdq_insert_after(item, new_item0);
	else
		new_item0 = cmdq_append(NULL, new_item0);
	cmd_list_free(pr->cmdlist);
	cmdq_free_state(state);

	if (new_item != NULL)
		*new_item = new_item0;
	return (0);
}

void
cfg_add_cause(const char *fmt, ...)
{
	va_list	 ap;
	char	*msg;

	va_start(ap, fmt);
	xvasprintf(&msg, fmt, ap);
	va_end(ap);

	cfg_ncauses++;
	cfg_causes = xreallocarray(cfg_causes, cfg_ncauses, sizeof *cfg_causes);
	cfg_causes[cfg_ncauses - 1] = msg;
}

void
cfg_print_causes(struct cmdq_item *item)
{
	struct client	*c = cmdq_get_client(item);
	u_int		 i;

	for (i = 0; i < cfg_ncauses; i++) {
		if (c != NULL && (c->flags & CLIENT_CONTROL))
			control_write(c, "%%config-error %s", cfg_causes[i]);
		else
			cmdq_print(item, "%s", cfg_causes[i]);
		free(cfg_causes[i]);
	}

	free(cfg_causes);
	cfg_causes = NULL;
	cfg_ncauses = 0;
}

void
cfg_show_causes(struct session *s)
{
	struct client			*c = TAILQ_FIRST(&clients);
	struct window_pane		*wp;
	struct window_mode_entry	*wme;
	u_int				 i;

	if (cfg_ncauses == 0)
		return;

	if (c != NULL && (c->flags & CLIENT_CONTROL)) {
		for (i = 0; i < cfg_ncauses; i++) {
			control_write(c, "%%config-error %s", cfg_causes[i]);
			free(cfg_causes[i]);
		}
		goto out;
	}

	if (s == NULL) {
		if (c != NULL && c->session != NULL)
			s = c->session;
		else
			s = RB_MIN(sessions, &sessions);
	}
	if (s == NULL || s->attached == 0) /* wait for an attached session */
		return;
	wp = s->curw->window->active;

	wme = TAILQ_FIRST(&wp->modes);
	if (wme == NULL || wme->mode != &window_view_mode)
		window_pane_set_mode(wp, NULL, &window_view_mode, NULL, NULL);
	for (i = 0; i < cfg_ncauses; i++) {
		window_copy_add(wp, 0, "%s", cfg_causes[i]);
		free(cfg_causes[i]);
	}

out:
	free(cfg_causes);
	cfg_causes = NULL;
	cfg_ncauses = 0;
}

/*
 * Plugin management functions for tpm integration
 * Embedded in cfg.c to minimize new file creation
 */

#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <sys/wait.h>

/* Global plugin tree */
struct plugins plugins = RB_INITIALIZER(&plugins);

/* Plugin comparison function for RB tree */
int
plugin_cmp(struct plugin *a, struct plugin *b)
{
	return strcmp(a->name, b->name);
}

/* Generate RB tree functions */
RB_GENERATE(plugins, plugin, entry, plugin_cmp);

/* Find plugin by name */
struct plugin *
plugin_find(const char *name)
{
	struct plugin find;
	find.name = (char *)name;
	return RB_FIND(plugins, &plugins, &find);
}

/* Find plugin by source URL */
struct plugin *
plugin_find_by_source(const char *source)
{
	struct plugin *p;
	RB_FOREACH(p, plugins, &plugins) {
		if (strcmp(p->source, source) == 0)
			return p;
	}
	return NULL;
}

/* Create new plugin */
struct plugin *
plugin_create(const char *name, const char *source, const char *branch, struct project *project)
{
	struct plugin *p;

	p = xcalloc(1, sizeof *p);
	p->name = xstrdup(name);
	p->source = xstrdup(source);
	if (branch != NULL)
		p->branch = xstrdup(branch);

	p->project = project;
	p->is_global = (project == NULL);
	p->status = PLUGIN_STATUS_PARSED;

	gettimeofday(&p->parsed_time, NULL);

	p->install_path = plugin_get_install_path(p);

	RB_INSERT(plugins, &plugins, p);

	return p;
}

/* Destroy plugin */
void
plugin_destroy(struct plugin *p)
{
	RB_REMOVE(plugins, &plugins, p);

	free(p->name);
	free(p->source);
	free(p->branch);
	free(p->install_path);
	free(p->last_error);

	free(p);
}

/* Get install path for plugin */
char *
plugin_get_install_path(struct plugin *p)
{
	char *path;
	const char *home = getenv("HOME");

	if (home == NULL)
		home = "/tmp";

	if (p->is_global) {
		xasprintf(&path, "%s/.smux/plugins/global/%s", home, p->name);
	} else {
		xasprintf(&path, "%s/.smux/plugins/%s/%s", home, p->project->name, p->name);
	}

	return path;
}

/* Parse plugin declaration and create plugin entry */
int
plugin_parse_declaration(const char *value, struct project *project)
{
	char *source, *branch = NULL, *name;
	char *val_copy, *hash_pos;
	struct plugin *existing;

	/* Make a copy for parsing */
	val_copy = xstrdup(value);

	/* Check for branch specification (plugin#branch) */
	hash_pos = strchr(val_copy, '#');
	if (hash_pos != NULL) {
		*hash_pos = '\0';
		branch = hash_pos + 1;
	}

	source = val_copy;

	/* Extract plugin name from source */
	char *slash = strrchr(source, '/');
	if (slash != NULL) {
		name = slash + 1;
	} else {
		name = source;
	}

	/* Check if plugin already exists */
	existing = plugin_find(name);
	if (existing != NULL) {
		/* Update existing plugin */
		free(existing->source);
		existing->source = xstrdup(source);

		free(existing->branch);
		existing->branch = branch ? xstrdup(branch) : NULL;

		existing->status = PLUGIN_STATUS_PARSED;
		gettimeofday(&existing->parsed_time, NULL);
	} else {
		/* Create new plugin */
		plugin_create(name, source, branch, project);
	}

	free(val_copy);
	return 0;
}

/* Initialize plugin directories */
void
plugin_init_directories(void)
{
	char *base_path, *global_path;
	const char *home = getenv("HOME");
	struct stat st;

	if (home == NULL)
		return;

	/* Create base plugin directory */
	xasprintf(&base_path, "%s/.smux/plugins", home);
	if (stat(base_path, &st) != 0 && mkdir(base_path, 0755) != 0 && errno != EEXIST) {
		log_debug("failed to create plugin directory %s: %s", base_path, strerror(errno));
	}

	/* Create global plugin directory */
	xasprintf(&global_path, "%s/global", base_path);
	if (stat(global_path, &st) != 0 && mkdir(global_path, 0755) != 0 && errno != EEXIST) {
		log_debug("failed to create global plugin directory %s: %s", global_path, strerror(errno));
	}

	free(base_path);
	free(global_path);
}

/* Install plugin using git clone */
int
plugin_install(struct plugin *p)
{
	char *cmd, *git_url;
	int status;
	pid_t pid;
	struct stat st;

	/* Check if already installed */
	if (stat(p->install_path, &st) == 0) {
		p->status = PLUGIN_STATUS_INSTALLED;
		return 0;
	}

	p->status = PLUGIN_STATUS_INSTALLING;

	/* Convert GitHub shorthand to full URL if needed */
	if (strchr(p->source, '/') != NULL && strstr(p->source, "://") == NULL) {
		xasprintf(&git_url, "https://github.com/%s", p->source);
	} else {
		git_url = xstrdup(p->source);
	}

	/* Prepare git clone command */
	if (p->branch != NULL) {
		xasprintf(&cmd, "git clone --single-branch --branch %s --recursive %s %s",
		         p->branch, git_url, p->install_path);
	} else {
		xasprintf(&cmd, "git clone --single-branch --recursive %s %s",
		         git_url, p->install_path);
	}

	/* Execute git clone */
	pid = fork();
	if (pid == 0) {
		/* Child process */
		execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
		_exit(127);
	} else if (pid > 0) {
		/* Parent process */
		waitpid(pid, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			p->status = PLUGIN_STATUS_INSTALLED;
			gettimeofday(&p->install_time, NULL);
		} else {
			p->status = PLUGIN_STATUS_FAILED;
			if (p->last_error) free(p->last_error);
			xasprintf(&p->last_error, "git clone failed with exit code %d", WEXITSTATUS(status));
		}
	} else {
		p->status = PLUGIN_STATUS_FAILED;
		if (p->last_error) free(p->last_error);
		xasprintf(&p->last_error, "fork failed: %s", strerror(errno));
	}

	free(cmd);
	free(git_url);

	return (p->status == PLUGIN_STATUS_INSTALLED) ? 0 : -1;
}

/* Update plugin using git pull */
int
plugin_update(struct plugin *p)
{
	char *cmd;
	int status;
	pid_t pid;

	p->status = PLUGIN_STATUS_UPDATING;

	/* Prepare git pull command */
	xasprintf(&cmd, "cd %s && git pull", p->install_path);

	/* Execute git pull */
	pid = fork();
	if (pid == 0) {
		/* Child process */
		execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
		_exit(127);
	} else if (pid > 0) {
		/* Parent process */
		waitpid(pid, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			p->status = PLUGIN_STATUS_INSTALLED;
		} else {
			p->status = PLUGIN_STATUS_FAILED;
			if (p->last_error) free(p->last_error);
			xasprintf(&p->last_error, "git pull failed with exit code %d", WEXITSTATUS(status));
		}
	} else {
		p->status = PLUGIN_STATUS_FAILED;
		if (p->last_error) free(p->last_error);
		xasprintf(&p->last_error, "fork failed: %s", strerror(errno));
	}

	free(cmd);

	return (p->status == PLUGIN_STATUS_INSTALLED) ? 0 : -1;
}

/* Remove plugin directory */
int
plugin_remove(struct plugin *p)
{
	char *cmd;
	int status;
	pid_t pid;

	p->status = PLUGIN_STATUS_REMOVING;

	/* Prepare remove command */
	xasprintf(&cmd, "rm -rf %s", p->install_path);

	/* Execute remove command */
	pid = fork();
	if (pid == 0) {
		/* Child process */
		execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
		_exit(127);
	} else if (pid > 0) {
		/* Parent process */
		waitpid(pid, &status, 0);
	}

	free(cmd);

	/* Remove from plugin tree */
	plugin_destroy(p);

	return 0;
}

/* Source plugin configuration files */
void
plugin_source(struct plugin *p)
{
	char *tmux_file;
	struct stat st;

	if (p->status != PLUGIN_STATUS_INSTALLED)
		return;

	/* Look for .tmux file in plugin directory */
	xasprintf(&tmux_file, "%s/%s.tmux", p->install_path, p->name);
	if (stat(tmux_file, &st) == 0) {
		load_cfg(tmux_file, NULL, NULL, NULL, CMD_PARSE_QUIET, NULL);
	}
	free(tmux_file);

	/* Also try just .tmux */
	xasprintf(&tmux_file, "%s/.tmux", p->install_path);
	if (stat(tmux_file, &st) == 0) {
		load_cfg(tmux_file, NULL, NULL, NULL, CMD_PARSE_QUIET, NULL);
	}
	free(tmux_file);
}

/* Source all installed plugins */
void
plugin_source_all(void)
{
	struct plugin *p;
	RB_FOREACH(p, plugins, &plugins) {
		plugin_source(p);
	}
}

/* Cleanup plugin system */
void
plugin_cleanup(void)
{
	struct plugin *p, *tmp;
	RB_FOREACH_SAFE(p, plugins, &plugins, tmp) {
		plugin_destroy(p);
	}
}
