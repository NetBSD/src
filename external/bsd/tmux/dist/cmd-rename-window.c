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

#include <stdlib.h>

#include "tmux.h"

/*
 * Rename a window.
 */

static enum cmd_retval	cmd_rename_window_exec(struct cmd *,
			    struct cmdq_item *);

const struct cmd_entry cmd_rename_window_entry = {
	.name = "rename-window",
	.alias = "renamew",

	.args = { "t:", 1, 1 },
	.usage = CMD_TARGET_WINDOW_USAGE " new-name",

	.target = { 't', CMD_FIND_WINDOW, 0 },

	.flags = CMD_AFTERHOOK,
	.exec = cmd_rename_window_exec
};

static enum cmd_retval
cmd_rename_window_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args	*args = self->args;
	struct client	*c = cmd_find_client(item, NULL, 1);
	struct session	*s = item->target.s;
	struct winlink	*wl = item->target.wl;
	char		*newname;

	newname = format_single(item, args->argv[0], c, s, wl, NULL);
	window_set_name(wl->window, newname);
	options_set_number(wl->window->options, "automatic-rename", 0);

	server_status_window(wl->window);
	free(newname);

	return (CMD_RETURN_NORMAL);
}
