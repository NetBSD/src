/* $OpenBSD$ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicm@users.sourceforge.net>
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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tmux.h"

/*
 * Create a new window.
 */

#define NEW_WINDOW_TEMPLATE "#{session_name}:#{window_index}.#{pane_index}"

static enum cmd_retval	cmd_new_window_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_new_window_entry = {
	"new-window", "neww",
	"ac:dF:kn:Pt:", 0, -1,
	"[-adkP] [-c start-directory] [-F format] [-n window-name] "
	CMD_TARGET_WINDOW_USAGE " [command]",
	0,
	cmd_new_window_exec
};

static enum cmd_retval
cmd_new_window_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = self->args;
	struct session		*s = item->state.tflag.s;
	struct winlink		*wl = item->state.tflag.wl;
	struct client		*c = item->state.c;
	int			 idx = item->state.tflag.idx;
	const char		*cmd, *path, *template, *cwd, *to_free;
	char		       **argv, *cause, *cp;
	int			 argc, detached;
	struct environ_entry	*envent;
	struct cmd_find_state	 fs;

	if (args_has(args, 'a')) {
		wl = cmd_find_window(cmdq, args_get(args, 't'), &s);
		if (wl == NULL)
			return (CMD_RETURN_ERROR);
		if ((idx = winlink_shuffle_up(s, wl)) == -1) {
			cmdq_error(item, "no free window indexes");
			return (CMD_RETURN_ERROR);
		}
	} else {
		idx = cmd_find_index(cmdq, args_get(args, 't'), &s);
		if (idx == -2)
			return (CMD_RETURN_ERROR);
	}
	detached = args_has(args, 'd');

	if (args->argc == 0) {
		cmd = options_get_string(&s->options, "default-command");
		if (cmd != NULL && *cmd != '\0') {
			argc = 1;
			argv = __UNCONST(&cmd);
		} else {
			argc = 0;
			argv = NULL;
		}
	} else {
		argc = args->argc;
		argv = args->argv;
	}

	path = NULL;
	if (item->client != NULL && item->client->session == NULL)
		envent = environ_find(item->client->environ, "PATH");
	else
		envent = environ_find(&s->environ, "PATH");
	if (envent != NULL)
		path = envent->value;

	if (args_has(args, 'c')) {
		cwd = args_get(args, 'c');
		to_free = cwd = format_single(item, cwd, c, s, NULL, NULL);
	} else if (item->client != NULL && item->client->session == NULL)
		cwd = item->client->cwd;
	else
		cwd = s->cwd;

	wl = NULL;
	if (idx != -1)
		wl = winlink_find_by_index(&s->windows, idx);
	if (wl != NULL && args_has(args, 'k')) {
		/*
		 * Can't use session_detach as it will destroy session if this
		 * makes it empty.
		 */
		notify_session_window("window-unlinked", s, wl->window);
		wl->flags &= ~WINLINK_ALERTFLAGS;
		winlink_stack_remove(&s->lastw, wl);
		winlink_remove(&s->windows, wl);

		/* Force select/redraw if current. */
		if (wl == s->curw) {
			detached = 0;
			s->curw = NULL;
		}
	}

	if (idx == -1)
		idx = -1 - options_get_number(&s->options, "base-index");
	wl = session_new(s, args_get(args, 'n'), argc, argv, path, cwd, idx,
		&cause);
	if (wl == NULL) {
		cmdq_error(item, "create window failed: %s", cause);
		free(cause);
		goto error;
	}
	if (!detached) {
		session_select(s, wl->idx);
		server_redraw_session_group(s);
	} else
		server_status_session_group(s);

	if (args_has(args, 'P')) {
		if ((template = args_get(args, 'F')) == NULL)
			template = NEW_WINDOW_TEMPLATE;
		cp = format_single(item, template, c, s, wl, NULL);
		cmdq_print(item, "%s", cp);
		free(cp);
	}

	if (to_free != NULL)
		free(__UNCONST(to_free));

	cmd_find_from_winlink(&fs, s, wl);
	hooks_insert(s->hooks, item, &fs, "after-new-window");

	return (CMD_RETURN_NORMAL);

error:
	if (fd != -1)
		close(fd);
	return (CMD_RETURN_ERROR);
}
