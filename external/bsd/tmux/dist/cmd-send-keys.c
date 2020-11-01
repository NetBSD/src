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

#include <stdlib.h>
#include <string.h>

#include "tmux.h"

/*
 * Send keys to client.
 */

static enum cmd_retval	cmd_send_keys_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_send_keys_entry = {
	.name = "send-keys",
	.alias = "send",

	.args = { "FHlMN:Rt:X", 0, -1 },
	.usage = "[-FHlMRX] [-N repeat-count] " CMD_TARGET_PANE_USAGE
	         " key ...",

	.target = { 't', CMD_FIND_PANE, 0 },

	.flags = CMD_AFTERHOOK,
	.exec = cmd_send_keys_exec
};

const struct cmd_entry cmd_send_prefix_entry = {
	.name = "send-prefix",
	.alias = NULL,

	.args = { "2t:", 0, 0 },
	.usage = "[-2] " CMD_TARGET_PANE_USAGE,

	.target = { 't', CMD_FIND_PANE, 0 },

	.flags = CMD_AFTERHOOK,
	.exec = cmd_send_keys_exec
};

static struct cmdq_item *
cmd_send_keys_inject_key(struct client *c, struct cmd_find_state *fs,
    struct cmdq_item *item, key_code key)
{
	struct session			*s = fs->s;
	struct winlink			*wl = fs->wl;
	struct window_pane		*wp = fs->wp;
	struct window_mode_entry	*wme;
	struct key_table		*table;
	struct key_binding		*bd;

	wme = TAILQ_FIRST(&fs->wp->modes);
	if (wme == NULL || wme->mode->key_table == NULL) {
		if (options_get_number(fs->wp->window->options, "xterm-keys"))
			key |= KEYC_XTERM;
		if (window_pane_key(wp, item->client, s, wl, key, NULL) != 0)
			return (NULL);
		return (item);
	}
	table = key_bindings_get_table(wme->mode->key_table(wme), 1);

	bd = key_bindings_get(table, key & ~KEYC_XTERM);
	if (bd != NULL) {
		table->references++;
		item = key_bindings_dispatch(bd, item, c, NULL, &item->target);
		key_bindings_unref_table(table);
	}
	return (item);
}

static struct cmdq_item *
cmd_send_keys_inject_string(struct client *c, struct cmd_find_state *fs,
    struct cmdq_item *item, struct args *args, int i)
{
	const char		*s = args->argv[i];
	struct cmdq_item	*new_item;
	struct utf8_data	*ud, *uc;
	wchar_t			 wc;
	key_code		 key;
	char			*endptr;
	long			 n;
	int			 literal;

	if (args_has(args, 'H')) {
		n = strtol(s, &endptr, 16);
		if (*s =='\0' || n < 0 || n > 0xff || *endptr != '\0')
			return (item);
		return (cmd_send_keys_inject_key(c, fs, item, KEYC_LITERAL|n));
	}

	literal = args_has(args, 'l');
	if (!literal) {
		key = key_string_lookup_string(s);
		if (key != KEYC_NONE && key != KEYC_UNKNOWN) {
			new_item = cmd_send_keys_inject_key(c, fs, item, key);
			if (new_item != NULL)
				return (new_item);
		}
		literal = 1;
	}
	if (literal) {
		ud = utf8_fromcstr(s);
		for (uc = ud; uc->size != 0; uc++) {
			if (utf8_combine(uc, &wc) != UTF8_DONE)
				continue;
			item = cmd_send_keys_inject_key(c, fs, item, wc);
		}
		free(ud);
	}
	return (item);
}

static enum cmd_retval
cmd_send_keys_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args			*args = self->args;
	struct client			*c = cmd_find_client(item, NULL, 1);
	struct cmd_find_state		*fs = &item->target;
	struct window_pane		*wp = item->target.wp;
	struct session			*s = item->target.s;
	struct winlink			*wl = item->target.wl;
	struct mouse_event		*m = &item->shared->mouse;
	struct window_mode_entry	*wme = TAILQ_FIRST(&wp->modes);
	int				 i;
	key_code			 key;
	u_int				 np = 1;
	char				*cause = NULL;

	if (args_has(args, 'N')) {
		np = args_strtonum(args, 'N', 1, UINT_MAX, &cause);
		if (cause != NULL) {
			cmdq_error(item, "repeat count %s", cause);
			free(cause);
			return (CMD_RETURN_ERROR);
		}
		if (wme != NULL && (args_has(args, 'X') || args->argc == 0)) {
			if (wme == NULL || wme->mode->command == NULL) {
				cmdq_error(item, "not in a mode");
				return (CMD_RETURN_ERROR);
			}
			wme->prefix = np;
		}
	}

	if (args_has(args, 'X')) {
		if (wme == NULL || wme->mode->command == NULL) {
			cmdq_error(item, "not in a mode");
			return (CMD_RETURN_ERROR);
		}
		if (!m->valid)
			m = NULL;
		wme->mode->command(wme, c, s, wl, args, m);
		return (CMD_RETURN_NORMAL);
	}

	if (args_has(args, 'M')) {
		wp = cmd_mouse_pane(m, &s, NULL);
		if (wp == NULL) {
			cmdq_error(item, "no mouse target");
			return (CMD_RETURN_ERROR);
		}
		window_pane_key(wp, item->client, s, wl, m->key, m);
		return (CMD_RETURN_NORMAL);
	}

	if (self->entry == &cmd_send_prefix_entry) {
		if (args_has(args, '2'))
			key = options_get_number(s->options, "prefix2");
		else
			key = options_get_number(s->options, "prefix");
		cmd_send_keys_inject_key(c, fs, item, key);
		return (CMD_RETURN_NORMAL);
	}

	if (args_has(args, 'R')) {
		window_pane_reset_palette(wp);
		input_reset(wp, 1);
	}

	for (; np != 0; np--) {
		for (i = 0; i < args->argc; i++)
			item = cmd_send_keys_inject_string(c, fs, item, args, i);
	}

	return (CMD_RETURN_NORMAL);
}
