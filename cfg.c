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
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <glob.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

	/* Load plugins after configuration is processed */
	/* plugin_source_all(c ? c->session : NULL); */ /* Temporarily disabled to debug hang */

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

/* Plugin management functions */
struct plugins plugins_global = RB_INITIALIZER(&plugins_global);
struct plugins plugins_project = RB_INITIALIZER(&plugins_project);

int
plugin_cmp(struct plugin *a, struct plugin *b)
{
	return strcmp(a->name, b->name);
}

RB_GENERATE(plugins, plugin, entry, plugin_cmp);

struct plugin *
plugin_find(const char *name, int scope)
{
	struct plugin find;
	find.name = (char *)name;

	if (scope == PLUGIN_GLOBAL)
		return RB_FIND(plugins, &plugins_global, &find);
	else
		return RB_FIND(plugins, &plugins_project, &find);
}

struct plugin *
plugin_create(const char *name, const char *source, int scope)
{
	struct plugin *plugin;

	plugin = xcalloc(1, sizeof *plugin);
	plugin->name = xstrdup(name);
	plugin->source = source ? xstrdup(source) : NULL;
	plugin->branch = NULL;
	plugin->path = NULL;
	plugin->scope = scope;
	plugin->status = PLUGIN_STATUS_DECLARED;
	plugin->error_msg = NULL;
	gettimeofday(&plugin->install_time, NULL);

	if (scope == PLUGIN_GLOBAL)
		RB_INSERT(plugins, &plugins_global, plugin);
	else
		RB_INSERT(plugins, &plugins_project, plugin);

	return plugin;
}

void
plugin_destroy(struct plugin *plugin)
{
	if (plugin->scope == PLUGIN_GLOBAL)
		RB_REMOVE(plugins, &plugins_global, plugin);
	else
		RB_REMOVE(plugins, &plugins_project, plugin);

	free(plugin->name);
	free(plugin->source);
	free(plugin->branch);
	free(plugin->path);
	free(plugin->error_msg);
	free(plugin);
}

void
plugin_set_path(struct plugin *plugin, const char *path)
{
	free(plugin->path);
	plugin->path = path ? xstrdup(path) : NULL;
}

void
plugin_set_status(struct plugin *plugin, int status, const char *error_msg)
{
	plugin->status = status;
	free(plugin->error_msg);
	plugin->error_msg = error_msg ? xstrdup(error_msg) : NULL;
}

int
plugin_parse_declaration(const char *declaration, char **name, char **source, char **branch)
{
	char *decl_copy, *token, *hash_pos;

	*name = *source = *branch = NULL;

	decl_copy = xstrdup(declaration);

	/* Look for branch specification (after #) */
	hash_pos = strchr(decl_copy, '#');
	if (hash_pos != NULL) {
		*hash_pos = '\0';
		*branch = xstrdup(hash_pos + 1);
	}

	/* Extract source */
	*source = xstrdup(decl_copy);

	/* Extract plugin name from source */
	token = strrchr(decl_copy, '/');
	if (token != NULL) {
		*name = xstrdup(token + 1);
	} else {
		*name = xstrdup(decl_copy);
	}

	free(decl_copy);
	return 0;
}

const char *
plugin_get_path(int scope, struct session *s)
{
	static char path[PATH_MAX];
	const char *home;

	home = find_home();
	if (home == NULL)
		return NULL;

	if (scope == PLUGIN_GLOBAL) {
		snprintf(path, sizeof path, "%s/.smux/plugins/global", home);
	} else if (s && s->project) {
		snprintf(path, sizeof path, "%s/.smux/plugins/%s", home, s->project->name);
	} else {
		snprintf(path, sizeof path, "%s/.smux/plugins/global", home);
	}

	return path;
}

void
plugin_init(void)
{
	/* Plugin initialization - create directories if needed */
	const char *home;
	char path[PATH_MAX];

	home = find_home();
	if (home == NULL)
		return;

	/* Create global plugin directory */
	snprintf(path, sizeof path, "%s/.smux/plugins", home);
	mkdir(path, 0755);

	snprintf(path, sizeof path, "%s/.smux/plugins/global", home);
	mkdir(path, 0755);
}

int
plugin_install(struct plugin *plugin)
{
	char cmd[1024];
	char plugin_dir[PATH_MAX];
	const char *base_path;
	int ret;

	if (!plugin || !plugin->source || !plugin->path)
		return -1;

	base_path = plugin->path;
	snprintf(plugin_dir, sizeof plugin_dir, "%s/%s", base_path, plugin->name);

	/* Create plugin base directory if it doesn't exist */
	mkdir(base_path, 0755);

	/* Check if plugin already exists */
	if (access(plugin_dir, F_OK) == 0) {
		log_debug("Plugin %s already exists at %s", plugin->name, plugin_dir);
		return 0;
	}

	plugin_set_status(plugin, PLUGIN_STATUS_INSTALLING, NULL);

	/* Construct git clone command */
	if (strstr(plugin->source, "://") != NULL) {
		/* Full git URL */
		if (plugin->branch) {
			snprintf(cmd, sizeof cmd,
			    "git clone --depth 1 --branch %s --recursive --quiet \"%s\" \"%s\" 2>/dev/null",
			    plugin->branch, plugin->source, plugin_dir);
		} else {
			snprintf(cmd, sizeof cmd,
			    "git clone --depth 1 --recursive --quiet \"%s\" \"%s\" 2>/dev/null",
			    plugin->source, plugin_dir);
		}
	} else {
		/* GitHub shorthand (user/repo) */
		if (plugin->branch) {
			snprintf(cmd, sizeof cmd,
			    "git clone --depth 1 --branch %s --recursive --quiet \"https://github.com/%s.git\" \"%s\" 2>/dev/null",
			    plugin->branch, plugin->source, plugin_dir);
		} else {
			snprintf(cmd, sizeof cmd,
			    "git clone --depth 1 --recursive --quiet \"https://github.com/%s.git\" \"%s\" 2>/dev/null",
			    plugin->source, plugin_dir);
		}
	}

	log_debug("Installing plugin %s: %s", plugin->name, cmd);
	ret = system(cmd);

	if (ret == 0) {
		plugin_set_status(plugin, PLUGIN_STATUS_INSTALLED, NULL);
		log_debug("Successfully installed plugin %s", plugin->name);
		return 0;
	} else {
		plugin_set_status(plugin, PLUGIN_STATUS_ERROR, "Git clone failed");
		log_debug("Failed to install plugin %s (exit code: %d)", plugin->name, ret);
		return -1;
	}
}

int
plugin_update(struct plugin *plugin)
{
	char cmd[1024];
	char plugin_dir[PATH_MAX];
	int ret;

	if (!plugin || !plugin->path || !plugin->name)
		return -1;

	snprintf(plugin_dir, sizeof plugin_dir, "%s/%s", plugin->path, plugin->name);

	/* Check if plugin directory exists */
	if (access(plugin_dir, F_OK) != 0) {
		log_debug("Plugin %s directory does not exist: %s", plugin->name, plugin_dir);
		return plugin_install(plugin);
	}

	/* Change to plugin directory and pull updates */
	snprintf(cmd, sizeof cmd,
	    "cd \"%s\" && git pull --quiet 2>/dev/null",
	    plugin_dir);

	log_debug("Updating plugin %s: %s", plugin->name, cmd);
	ret = system(cmd);

	if (ret == 0) {
		log_debug("Successfully updated plugin %s", plugin->name);
		return 0;
	} else {
		log_debug("Failed to update plugin %s (exit code: %d)", plugin->name, ret);
		return -1;
	}
}

int
plugin_remove(struct plugin *plugin)
{
	char cmd[1024];
	char plugin_dir[PATH_MAX];
	int ret;

	if (!plugin || !plugin->path || !plugin->name)
		return -1;

	snprintf(plugin_dir, sizeof plugin_dir, "%s/%s", plugin->path, plugin->name);

	/* Check if plugin directory exists */
	if (access(plugin_dir, F_OK) != 0) {
		log_debug("Plugin %s directory does not exist: %s", plugin->name, plugin_dir);
		return 0; /* Already removed */
	}

	/* Remove plugin directory */
	snprintf(cmd, sizeof cmd, "rm -rf \"%s\" 2>/dev/null", plugin_dir);

	log_debug("Removing plugin %s: %s", plugin->name, cmd);
	ret = system(cmd);

	if (ret == 0) {
		log_debug("Successfully removed plugin %s", plugin->name);
		return 0;
	} else {
		log_debug("Failed to remove plugin %s (exit code: %d)", plugin->name, ret);
		return -1;
	}
}

void
plugin_source_all(struct session *s)
{
	struct plugin *plugin;
	char plugin_dir[PATH_MAX];
	char tmux_file[PATH_MAX];
	char glob_pattern[PATH_MAX];
	char cmd[1024];
	glob_t glob_buf;
	int i;

	/* Source global plugins first */
	RB_FOREACH(plugin, plugins, &plugins_global) {
		if ((plugin->status == PLUGIN_STATUS_INSTALLED || plugin->status == PLUGIN_STATUS_DECLARED) && plugin->path && plugin->name) {
			snprintf(plugin_dir, sizeof plugin_dir, "%s/%s", plugin->path, plugin->name);
			snprintf(tmux_file, sizeof tmux_file, "%s/%s.tmux", plugin_dir, plugin->name);

			/* Check if plugin.tmux file exists (standard naming) */
			if (access(tmux_file, F_OK) == 0) {
				snprintf(cmd, sizeof cmd, "mkdir -p /tmp/smux-bin && ln -sf \"$(which smux)\" /tmp/smux-bin/tmux 2>/dev/null; export PATH=\"/tmp/smux-bin:$PATH\" && bash \"%s\" 2>/dev/null", tmux_file);
				log_debug("Sourcing global plugin %s: %s", plugin->name, tmux_file);
				/* system(cmd); */ /* Temporarily disabled to debug hang */
			} else {
				/* Use glob to find any .tmux file in the plugin directory */
				snprintf(glob_pattern, sizeof glob_pattern, "%s/*.tmux", plugin_dir);
				if (glob(glob_pattern, GLOB_ERR, NULL, &glob_buf) == 0) {
					for (i = 0; i < (int)glob_buf.gl_pathc; i++) {
						snprintf(cmd, sizeof cmd, "mkdir -p /tmp/smux-bin && ln -sf \"$(which smux)\" /tmp/smux-bin/tmux 2>/dev/null; export PATH=\"/tmp/smux-bin:$PATH\" && bash \"%s\" 2>/dev/null", glob_buf.gl_pathv[i]);
						log_debug("Sourcing global plugin %s: %s", plugin->name, glob_buf.gl_pathv[i]);
						/* system(cmd); */ /* Temporarily disabled to debug hang */
						break; /* Only source the first .tmux file found */
					}
					globfree(&glob_buf);
				} else {
					log_debug("No .tmux file found for global plugin %s", plugin->name);
				}
			}
		}
	}

	/* Source project plugins (if in project context) */
	if (s && s->project) {
		RB_FOREACH(plugin, plugins, &plugins_project) {
			if ((plugin->status == PLUGIN_STATUS_INSTALLED || plugin->status == PLUGIN_STATUS_DECLARED) && plugin->path && plugin->name) {
				snprintf(plugin_dir, sizeof plugin_dir, "%s/%s", plugin->path, plugin->name);
				snprintf(tmux_file, sizeof tmux_file, "%s/%s.tmux", plugin_dir, plugin->name);

				/* Check if plugin.tmux file exists (standard naming) */
				if (access(tmux_file, F_OK) == 0) {
					snprintf(cmd, sizeof cmd, "mkdir -p /tmp/smux-bin && ln -sf \"$(which smux)\" /tmp/smux-bin/tmux 2>/dev/null; export PATH=\"/tmp/smux-bin:$PATH\" && bash \"%s\" 2>/dev/null", tmux_file);
					log_debug("Sourcing project plugin %s: %s", plugin->name, tmux_file);
					/* system(cmd); */ /* Temporarily disabled to debug hang */
				} else {
					/* Use glob to find any .tmux file in the plugin directory */
					snprintf(glob_pattern, sizeof glob_pattern, "%s/*.tmux", plugin_dir);
					if (glob(glob_pattern, GLOB_ERR, NULL, &glob_buf) == 0) {
						for (i = 0; i < (int)glob_buf.gl_pathc; i++) {
							snprintf(cmd, sizeof cmd, "mkdir -p /tmp/smux-bin && ln -sf \"$(which smux)\" /tmp/smux-bin/tmux 2>/dev/null; export PATH=\"/tmp/smux-bin:$PATH\" && bash \"%s\" 2>/dev/null", glob_buf.gl_pathv[i]);
							log_debug("Sourcing project plugin %s: %s", plugin->name, glob_buf.gl_pathv[i]);
							/* system(cmd); */ /* Temporarily disabled to debug hang */
							break; /* Only source the first .tmux file found */
						}
						globfree(&glob_buf);
					} else {
						log_debug("No .tmux file found for project plugin %s", plugin->name);
					}
				}
			}
		}
	}
}

/* Plugin command implementations */
static enum cmd_retval	cmd_install_plugins_exec(struct cmd *, struct cmdq_item *);
static enum cmd_retval	cmd_update_plugins_exec(struct cmd *, struct cmdq_item *);
static enum cmd_retval	cmd_remove_plugins_exec(struct cmd *, struct cmdq_item *);
static enum cmd_retval	cmd_list_plugins_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_install_plugins_entry = {
	.name = "install-plugins",
	.alias = "plugin-install",

	.args = { "q", 0, 0, NULL },
	.usage = "[-q]",

	.flags = CMD_STARTSERVER,
	.exec = cmd_install_plugins_exec
};

const struct cmd_entry cmd_update_plugins_entry = {
	.name = "update-plugins",
	.alias = "plugin-update",

	.args = { "q", 0, 0, NULL },
	.usage = "[-q]",

	.flags = CMD_STARTSERVER,
	.exec = cmd_update_plugins_exec
};

const struct cmd_entry cmd_remove_plugins_entry = {
	.name = "remove-plugins",
	.alias = "plugin-remove",

	.args = { "q", 0, 0, NULL },
	.usage = "[-q]",

	.flags = CMD_STARTSERVER,
	.exec = cmd_remove_plugins_exec
};

const struct cmd_entry cmd_list_plugins_entry = {
	.name = "list-plugins",
	.alias = "plugin-list",

	.args = { "F:", 0, 0, NULL },
	.usage = "[-F format]",

	.flags = CMD_STARTSERVER,
	.exec = cmd_list_plugins_exec
};

static enum cmd_retval
cmd_install_plugins_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args *args = cmd_get_args(self);
	struct plugin *plugin;
	int quiet = args_has(args, 'q');
	int installed = 0;

	/* Install global plugins */
	RB_FOREACH(plugin, plugins, &plugins_global) {
		if (plugin->status == PLUGIN_STATUS_DECLARED) {
			if (!quiet)
				cmdq_print(item, "Installing plugin: %s", plugin->name);

			if (plugin_install(plugin) == 0) {
				plugin_set_status(plugin, PLUGIN_STATUS_INSTALLED, NULL);
				installed++;
			} else {
				plugin_set_status(plugin, PLUGIN_STATUS_ERROR, "Installation failed");
				cmdq_error(item, "Failed to install plugin: %s", plugin->name);
			}
		}
	}

	/* Install project plugins */
	RB_FOREACH(plugin, plugins, &plugins_project) {
		if (plugin->status == PLUGIN_STATUS_DECLARED) {
			if (!quiet)
				cmdq_print(item, "Installing plugin: %s", plugin->name);

			if (plugin_install(plugin) == 0) {
				plugin_set_status(plugin, PLUGIN_STATUS_INSTALLED, NULL);
				installed++;
			} else {
				plugin_set_status(plugin, PLUGIN_STATUS_ERROR, "Installation failed");
				cmdq_error(item, "Failed to install plugin: %s", plugin->name);
			}
		}
	}

	if (!quiet)
		cmdq_print(item, "Installed %d plugins", installed);

	/* Source all installed plugins */
	if (installed > 0) {
		plugin_source_all(NULL);
	}

	return (CMD_RETURN_NORMAL);
}

static enum cmd_retval
cmd_update_plugins_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args *args = cmd_get_args(self);
	struct plugin *plugin;
	int quiet = args_has(args, 'q');
	int updated = 0;

	/* Update global plugins */
	RB_FOREACH(plugin, plugins, &plugins_global) {
		if (plugin->status == PLUGIN_STATUS_INSTALLED) {
			if (!quiet)
				cmdq_print(item, "Updating plugin: %s", plugin->name);

			if (plugin_update(plugin) == 0) {
				updated++;
			} else {
				cmdq_error(item, "Failed to update plugin: %s", plugin->name);
			}
		}
	}

	/* Update project plugins */
	RB_FOREACH(plugin, plugins, &plugins_project) {
		if (plugin->status == PLUGIN_STATUS_INSTALLED) {
			if (!quiet)
				cmdq_print(item, "Updating plugin: %s", plugin->name);

			if (plugin_update(plugin) == 0) {
				updated++;
			} else {
				cmdq_error(item, "Failed to update plugin: %s", plugin->name);
			}
		}
	}

	if (!quiet)
		cmdq_print(item, "Updated %d plugins", updated);

	return (CMD_RETURN_NORMAL);
}

static enum cmd_retval
cmd_remove_plugins_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args *args = cmd_get_args(self);
	int quiet = args_has(args, 'q');
	int removed = 0;

	/* TODO: Implement plugin removal logic */
	if (!quiet)
		cmdq_print(item, "Removed %d plugins", removed);

	return (CMD_RETURN_NORMAL);
}

static enum cmd_retval
cmd_list_plugins_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args *args = cmd_get_args(self);
	struct plugin *plugin;
	const char *template = args_get(args, 'F');

	if (template == NULL)
		template = "#{plugin_name}: #{plugin_source} [#{plugin_status}]";

	cmdq_print(item, "Global plugins:");
	RB_FOREACH(plugin, plugins, &plugins_global) {
		const char *status;
		switch (plugin->status) {
		case PLUGIN_STATUS_DECLARED:
			status = "declared";
			break;
		case PLUGIN_STATUS_INSTALLING:
			status = "installing";
			break;
		case PLUGIN_STATUS_INSTALLED:
			status = "installed";
			break;
		case PLUGIN_STATUS_ERROR:
			status = "error";
			break;
		default:
			status = "unknown";
			break;
		}
		cmdq_print(item, "  %s: %s [%s]", plugin->name,
		    plugin->source ? plugin->source : "unknown", status);
	}

	cmdq_print(item, "Project plugins:");
	RB_FOREACH(plugin, plugins, &plugins_project) {
		const char *status;
		switch (plugin->status) {
		case PLUGIN_STATUS_DECLARED:
			status = "declared";
			break;
		case PLUGIN_STATUS_INSTALLING:
			status = "installing";
			break;
		case PLUGIN_STATUS_INSTALLED:
			status = "installed";
			break;
		case PLUGIN_STATUS_ERROR:
			status = "error";
			break;
		default:
			status = "unknown";
			break;
		}
		cmdq_print(item, "  %s: %s [%s]", plugin->name,
		    plugin->source ? plugin->source : "unknown", status);
	}

	return (CMD_RETURN_NORMAL);
}
