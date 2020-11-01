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
#include <string.h>

#include "tmux.h"

/*
 * Show options.
 */

static enum cmd_retval	cmd_show_options_exec(struct cmd *, struct cmdq_item *);

static void		cmd_show_options_print(struct cmd *, struct cmdq_item *,
			    struct options_entry *, int, int);
static enum cmd_retval	cmd_show_options_all(struct cmd *, struct cmdq_item *,
			    int, struct options *);

const struct cmd_entry cmd_show_options_entry = {
	.name = "show-options",
	.alias = "show",

	.args = { "AgHpqst:vw", 0, 1 },
	.usage = "[-AgHpqsvw] " CMD_TARGET_PANE_USAGE " [option]",

	.target = { 't', CMD_FIND_PANE, CMD_FIND_CANFAIL },

	.flags = CMD_AFTERHOOK,
	.exec = cmd_show_options_exec
};

const struct cmd_entry cmd_show_window_options_entry = {
	.name = "show-window-options",
	.alias = "showw",

	.args = { "gvt:", 0, 1 },
	.usage = "[-gv] " CMD_TARGET_WINDOW_USAGE " [option]",

	.target = { 't', CMD_FIND_WINDOW, CMD_FIND_CANFAIL },

	.flags = CMD_AFTERHOOK,
	.exec = cmd_show_options_exec
};

const struct cmd_entry cmd_show_hooks_entry = {
	.name = "show-hooks",
	.alias = NULL,

	.args = { "gt:", 0, 1 },
	.usage = "[-g] " CMD_TARGET_SESSION_USAGE,

	.target = { 't', CMD_FIND_SESSION, 0 },

	.flags = CMD_AFTERHOOK,
	.exec = cmd_show_options_exec
};

static enum cmd_retval
cmd_show_options_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args			*args = self->args;
	struct cmd_find_state		*fs = &item->target;
	struct client			*c = cmd_find_client(item, NULL, 1);
	struct session			*s = item->target.s;
	struct winlink			*wl = item->target.wl;
	struct options			*oo;
	char				*argument, *name = NULL, *cause;
	int				 window, idx, ambiguous, parent, scope;
	struct options_entry		*o;

	window = (self->entry == &cmd_show_window_options_entry);

	if (args->argc == 0) {
		scope = options_scope_from_flags(args, window, fs, &oo, &cause);
		if (scope == OPTIONS_TABLE_NONE) {
			if (args_has(args, 'q'))
				return (CMD_RETURN_NORMAL);
			cmdq_error(item, "%s", cause);
			free(cause);
			return (CMD_RETURN_ERROR);
		}
		return (cmd_show_options_all(self, item, scope, oo));
	}
	argument = format_single(item, args->argv[0], c, s, wl, NULL);

	name = options_match(argument, &idx, &ambiguous);
	if (name == NULL) {
		if (args_has(args, 'q'))
			goto fail;
		if (ambiguous)
			cmdq_error(item, "ambiguous option: %s", argument);
		else
			cmdq_error(item, "invalid option: %s", argument);
		goto fail;
	}
	scope = options_scope_from_name(args, window, name, fs, &oo, &cause);
	if (scope == OPTIONS_TABLE_NONE) {
		if (args_has(args, 'q'))
			goto fail;
		cmdq_error(item, "%s", cause);
		free(cause);
		goto fail;
	}
	o = options_get_only(oo, name);
	if (args_has(args, 'A') && o == NULL) {
		o = options_get(oo, name);
		parent = 1;
	} else
		parent = 0;
	if (o != NULL)
		cmd_show_options_print(self, item, o, idx, parent);

	free(name);
	free(argument);
	return (CMD_RETURN_NORMAL);

fail:
	free(name);
	free(argument);
	return (CMD_RETURN_ERROR);
}

static void
cmd_show_options_print(struct cmd *self, struct cmdq_item *item,
    struct options_entry *o, int idx, int parent)
{
	struct options_array_item	*a;
	const char			*name = options_name(o);
	char				*value, *tmp = NULL, *escaped;

	if (idx != -1) {
		xasprintf(&tmp, "%s[%d]", name, idx);
		name = tmp;
	} else {
		if (options_isarray(o)) {
			a = options_array_first(o);
			if (a == NULL) {
				if (!args_has(self->args, 'v'))
					cmdq_print(item, "%s", name);
				return;
			}
			while (a != NULL) {
				idx = options_array_item_index(a);
				cmd_show_options_print(self, item, o, idx,
				    parent);
				a = options_array_next(a);
			}
			return;
		}
	}

	value = options_tostring(o, idx, 0);
	if (args_has(self->args, 'v'))
		cmdq_print(item, "%s", value);
	else if (options_isstring(o)) {
		escaped = args_escape(value);
		if (parent)
			cmdq_print(item, "%s* %s", name, escaped);
		else
			cmdq_print(item, "%s %s", name, escaped);
		free(escaped);
	} else {
		if (parent)
			cmdq_print(item, "%s* %s", name, value);
		else
			cmdq_print(item, "%s %s", name, value);
	}
	free(value);

	free(tmp);
}

static enum cmd_retval
cmd_show_options_all(struct cmd *self, struct cmdq_item *item, int scope,
    struct options *oo)
{
	const struct options_table_entry	*oe;
	struct options_entry			*o;
	struct options_array_item		*a;
	const char				*name;
	u_int					 idx;
	int					 parent;

	o = options_first(oo);
	while (o != NULL) {
		if (options_table_entry(o) == NULL)
			cmd_show_options_print(self, item, o, -1, 0);
		o = options_next(o);
	}
	for (oe = options_table; oe->name != NULL; oe++) {
		if (~oe->scope & scope)
			continue;

		if ((self->entry != &cmd_show_hooks_entry &&
		    !args_has(self->args, 'H') &&
		    oe != NULL &&
		    (oe->flags & OPTIONS_TABLE_IS_HOOK)) ||
		    (self->entry == &cmd_show_hooks_entry &&
		    (oe == NULL ||
		    (~oe->flags & OPTIONS_TABLE_IS_HOOK))))
			continue;

		o = options_get_only(oo, oe->name);
		if (o == NULL) {
			if (!args_has(self->args, 'A'))
				continue;
			o = options_get(oo, oe->name);
			if (o == NULL)
				continue;
			parent = 1;
		} else
			parent = 0;

		if (!options_isarray(o))
			cmd_show_options_print(self, item, o, -1, parent);
		else if ((a = options_array_first(o)) == NULL) {
			if (!args_has(self->args, 'v')) {
				name = options_name(o);
				if (parent)
					cmdq_print(item, "%s*", name);
				else
					cmdq_print(item, "%s", name);
			}
		} else {
			while (a != NULL) {
				idx = options_array_item_index(a);
				cmd_show_options_print(self, item, o, idx,
				    parent);
				a = options_array_next(a);
			}
		}
	}
	return (CMD_RETURN_NORMAL);
}
