/*	$NetBSD: mock_spawn_command.c,v 1.1.1.1 2026/05/09 18:39:22 christos Exp $	*/

/*++
/* NAME
/*	mock_spawn_command 3t
/* SUMMARY
/*	spawn_command() mocker
/* SYNOPSIS
/*	#include <mock_spawn_command.h>
/*
/*	typedef struct MOCK_SPAWN_CMD_REQ {
/*	struct spawn_args *want_args;
/*	int	out_status;
/*	const char *out_text;
P/*
/*	void	setup_mock_spawn_command(const MOCK_SPAWN_CMD_REQ *request)
/* DESCRIPTION
/*	setup_mock_spawn_command() instantiates a spawn_command() mocker,
/*	that given the expected inputs, will produce the specified
/*	outputs, without any additional dependencies or side effects.
/*
/*	This implementation supports only one instance of expectations
/*	and outputs, and makes a shallow copy of its inputs:
/* .IP want_args
/*	The inputs that the mock spawn_command() instance must
/*	receive. Otherwise, The mocker will terminate the test.
/* .IP out_status
/*	The spawn_command() result value.
/* .IP out_text
/*	Pointer to null-terminated text that will be written to the
/*	simulated command's standard error stream.
/* SEE ALSO
/*	spawn_command(3), command spawner
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <stringops.h>

 /*
  * Testing library.
  */
#include <mock_spawn_command.h>

 /*
  * Input expectations and outputs. For now we support only one instance.
  */
static MOCK_SPAWN_CMD_REQ mock_req;

void    setup_mock_spawn_command(const MOCK_SPAWN_CMD_REQ *request)
{

    /*
     * Validate the output requirements.
     */
    if (request->out_status == 0) {
	if (request->out_text != 0)
	    msg_fatal("setup_spawn_command: out_status is zero, but out_text "
		      "is not null");
    } else {
	if (request->out_text == 0 || *request->out_text == 0) {
	    msg_fatal("setup_spawn_command: out_status is not zero, but "
		      "out_text is null or empty");
	}
    }

    /*
     * Make a shallow copy of the arguments.
     */
    mock_req = *request;
}

 /*
  * Matchers. TODO(wietse)  factor out.
  */
typedef void PRINTFLIKE(1, 2) (*MATCH_WARN) (const char *,...);

#define STR_OR_NULL(s)          ((s) ? (s) : "(null)")
#define NOT_NULL_OR_NULL(arg)   ((arg) ? "(not null)" : "(null)")

/* match_decimal - match integers, report difference in decimal */

static bool match_decimal(const char *what, MATCH_WARN warn, int want, int got)
{
    if (want != got) {
	if (warn)
	    warn("%s: want %d, got %d", what, want, got);
	return (false);
    }
    return (true);
}

/* match_str - match null-terminated strings */

static int match_str(const char *what, MATCH_WARN warn,
		             const char *want, const char *got)
{
    if (want == 0 && got == 0)
	return (true);
    if (!want != !got) {
	if (warn)
	    warn("%s: want '%s', got '%s'", what, STR_OR_NULL(want),
		 STR_OR_NULL(got));
	return (false);
    }
    if (strcmp(got, want) != 0) {
	if (warn)
	    warn("%s: want %s, got %s", what, want, got);
	return (false);
    }
    return (true);
}

/* match_cpp - match (char **) instances */

static bool match_cpp(const char *what, MATCH_WARN warn,
		              char **want, char **got)
{
    char  **wpp, **gpp;

    if (want == 0 && got == 0)
	return (true);
    if (!want != !got) {
	if (warn)
	    warn("%s: want %s, got %s", what, NOT_NULL_OR_NULL(want),
		 NOT_NULL_OR_NULL(got));
	return (false);
    }
    for (wpp = want, gpp = got; /* void */ ; wpp++, gpp++) {
	if (*wpp == 0 && *gpp == 0) {
	    return (true);
	} else if (*wpp == 0 || *gpp == 0) {
	    if (warn)
		warn("%s: want '%s', got '%s'",
		     what, STR_OR_NULL(*wpp), STR_OR_NULL(*gpp));
	    return (false);
	} else if (!match_str(what, warn, *gpp, *wpp)) {
	    return (false);
	}
    }
    return (true);
}

/* match_spawn_args - compare expected against actual */

static bool match_spawn_args(MATCH_WARN warn, struct spawn_args *want,
			             struct spawn_args *got)
{
    return (match_cpp("argv", warn, want->argv, got->argv)
	    && match_str("command", warn,
			 want->command, got->command)
	    && match_decimal("uid", warn, want->uid, got->uid)
	    && match_cpp("env", warn, want->env, got->env)
	    && match_cpp("export", warn,
			 want->export, got->export)
	    && match_str("shell", warn, want->shell, got->shell)
	    && match_decimal("time_limit", warn,
			     want->time_limit, got->time_limit));
}

/* spawn_command - mock */

int     spawn_command(int key,...)
{
    struct spawn_args got_args;
    va_list ap;

    va_start(ap, key);
    get_spawn_args(&got_args, key, ap);
    va_end(ap);

    /*
     * Match spawn_command() arguments against expectations or terminate.
     */
    if (!match_spawn_args(msg_warn, &mock_req.want_args, &got_args))
	msg_fatal("spawn_command called with unexpected argument");

    /* Now output some info as if the command was executed. */
    if (mock_req.out_status != 0)
	write(got_args.stderr_fd, mock_req.out_text, strlen(mock_req.out_text));
    return (mock_req.out_status);
}
