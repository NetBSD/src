/* $OpenBSD$ */

/*
 * Copyright (c) 2009 Tiago Cunha <me@tiagocunha.org>
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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tmux.h"

/*
 * Saves a paste buffer to a file.
 */

static enum cmd_retval	cmd_save_buffer_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_save_buffer_entry = {
	.name = "save-buffer",
	.alias = "saveb",

	.args = { "ab:", 1, 1 },
	.usage = "[-a] " CMD_BUFFER_USAGE " path",

	.flags = CMD_AFTERHOOK,
	.exec = cmd_save_buffer_exec
};

const struct cmd_entry cmd_show_buffer_entry = {
	.name = "show-buffer",
	.alias = "showb",

	.args = { "b:", 0, 0 },
	.usage = CMD_BUFFER_USAGE,

	.flags = CMD_AFTERHOOK,
	.exec = cmd_save_buffer_exec
};

static void
cmd_save_buffer_done(__unused struct client *c, const char *path, int error,
    __unused int closed, __unused struct evbuffer *buffer, void *data)
{
	struct cmdq_item	*item = data;

	if (!closed)
		return;

	if (error != 0)
		cmdq_error(item, "%s: %s", path, strerror(error));
	cmdq_continue(item);
}

static enum cmd_retval
cmd_save_buffer_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = self->args;
	struct client		*c = cmd_find_client(item, NULL, 1);
	struct session		*s = item->target.s;
	struct winlink		*wl = item->target.wl;
	struct window_pane	*wp = item->target.wp;
	struct paste_buffer	*pb;
	int			 flags;
	const char		*bufname = args_get(args, 'b'), *bufdata;
	size_t			 bufsize;
	char			*path;

	if (bufname == NULL) {
		if ((pb = paste_get_top(NULL)) == NULL) {
			cmdq_error(item, "no buffers");
			return (CMD_RETURN_ERROR);
		}
	} else {
		pb = paste_get_name(bufname);
		if (pb == NULL) {
			cmdq_error(item, "no buffer %s", bufname);
			return (CMD_RETURN_ERROR);
		}
	}
	bufdata = paste_buffer_data(pb, &bufsize);

	if (self->entry == &cmd_show_buffer_entry)
		path = xstrdup("-");
	else
		path = format_single(item, args->argv[0], c, s, wl, wp);
	if (args_has(self->args, 'a'))
		flags = O_APPEND;
	else
		flags = 0;
	file_write(item->client, path, flags, bufdata, bufsize,
	    cmd_save_buffer_done, item);
	free(path);

	return (CMD_RETURN_WAIT);
}
