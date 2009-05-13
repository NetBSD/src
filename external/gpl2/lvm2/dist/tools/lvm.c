/*	$NetBSD: lvm.c,v 1.1.1.1.2.1 2009/05/13 18:52:47 jym Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tools.h"
#include "lvm2cmdline.h"

int main(int argc, char **argv)
{
	return lvm2_main(argc, argv);
}

#ifdef READLINE_SUPPORT

#  include <readline/readline.h>
#  include <readline/history.h>
#  ifndef HAVE_RL_COMPLETION_MATCHES
#    define rl_completion_matches(a, b) completion_matches((char *)a, b)
#  endif

static struct cmdline_context *_cmdline;

/* List matching commands */
static char *_list_cmds(const char *text, int state)
{
	static int i = 0;
	static size_t len = 0;

	/* Initialise if this is a new completion attempt */
	if (!state) {
		i = 0;
		len = strlen(text);
	}

	while (i < _cmdline->num_commands)
		if (!strncmp(text, _cmdline->commands[i++].name, len))
			return strdup(_cmdline->commands[i - 1].name);

	return NULL;
}

/* List matching arguments */
static char *_list_args(const char *text, int state)
{
	static int match_no = 0;
	static size_t len = 0;
	static struct command *com;

	/* Initialise if this is a new completion attempt */
	if (!state) {
		char *s = rl_line_buffer;
		int j = 0;

		match_no = 0;
		com = NULL;
		len = strlen(text);

		/* Find start of first word in line buffer */
		while (isspace(*s))
			s++;

		/* Look for word in list of commands */
		for (j = 0; j < _cmdline->num_commands; j++) {
			const char *p;
			char *q = s;

			p = _cmdline->commands[j].name;
			while (*p == *q) {
				p++;
				q++;
			}
			if ((!*p) && *q == ' ') {
				com = _cmdline->commands + j;
				break;
			}
		}

		if (!com)
			return NULL;
	}

	/* Short form arguments */
	if (len < 3) {
		while (match_no < com->num_args) {
			char s[3];
			char c;
			if (!(c = (_cmdline->the_args +
				   com->valid_args[match_no++])->short_arg))
				continue;

			sprintf(s, "-%c", c);
			if (!strncmp(text, s, len))
				return strdup(s);
		}
	}

	/* Long form arguments */
	if (match_no < com->num_args)
		match_no = com->num_args;

	while (match_no - com->num_args < com->num_args) {
		const char *l;
		l = (_cmdline->the_args +
		     com->valid_args[match_no++ - com->num_args])->long_arg;
		if (*(l + 2) && !strncmp(text, l, len))
			return strdup(l);
	}

	return NULL;
}

/* Custom completion function */
static char **_completion(const char *text, int start_pos,
			  int end_pos __attribute((unused)))
{
	char **match_list = NULL;
	int p = 0;

	while (isspace((int) *(rl_line_buffer + p)))
		p++;

	/* First word should be one of our commands */
	if (start_pos == p)
		match_list = rl_completion_matches(text, _list_cmds);

	else if (*text == '-')
		match_list = rl_completion_matches(text, _list_args);
	/* else other args */

	/* No further completion */
	rl_attempted_completion_over = 1;
	return match_list;
}

static int _hist_file(char *buffer, size_t size)
{
	char *e = getenv("HOME");

	if (dm_snprintf(buffer, size, "%s/.lvm_history", e) < 0) {
		log_error("$HOME/.lvm_history: path too long");
		return 0;
	}

	return 1;
}

static void _read_history(struct cmd_context *cmd)
{
	char hist_file[PATH_MAX];

	if (!_hist_file(hist_file, sizeof(hist_file)))
		return;

	if (read_history(hist_file))
		log_very_verbose("Couldn't read history from %s.", hist_file);

	stifle_history(find_config_tree_int(cmd, "shell/history_size",
				       DEFAULT_MAX_HISTORY));

}

static void _write_history(void)
{
	char hist_file[PATH_MAX];

	if (!_hist_file(hist_file, sizeof(hist_file)))
		return;

	if (write_history(hist_file))
		log_very_verbose("Couldn't write history to %s.", hist_file);
}

int lvm_shell(struct cmd_context *cmd, struct cmdline_context *cmdline)
{
	int argc, ret;
	char *input = NULL, *args[MAX_ARGS], **argv;

	rl_readline_name = "lvm";
	rl_attempted_completion_function = (CPPFunction *) _completion;

	_read_history(cmd);

	_cmdline = cmdline;

	_cmdline->interactive = 1;
	while (1) {
		free(input);
		input = readline("lvm> ");

		/* EOF */
		if (!input) {
			printf("\n");
			break;
		}

		/* empty line */
		if (!*input)
			continue;

		add_history(input);

		argv = args;

		if (lvm_split(input, &argc, argv, MAX_ARGS) == MAX_ARGS) {
			log_error("Too many arguments, sorry.");
			continue;
		}

		if (!strcmp(argv[0], "lvm")) {
			argv++;
			argc--;
		}

		if (!argc)
			continue;

		if (!strcmp(argv[0], "quit") || !strcmp(argv[0], "exit")) {
			remove_history(history_length - 1);
			log_error("Exiting.");
			break;
		}

		ret = lvm_run_command(cmd, argc, argv);
		if (ret == ENO_SUCH_CMD)
			log_error("No such command '%s'.  Try 'help'.",
				  argv[0]);

                if ((ret != ECMD_PROCESSED) && !error_message_produced()) {
			log_debug("Internal error: Failed command did not use log_error");
			log_error("Command failed with status code %d.", ret);
		}
		_write_history();
	}

	free(input);
	return 0;
}

#endif	/* READLINE_SUPPORT */
