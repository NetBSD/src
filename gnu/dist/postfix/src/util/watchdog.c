/*++
/* NAME
/*	watchdog 3
/* SUMMARY
/*	watchdog timer
/* SYNOPSIS
/*	#include <watchdog.h>
/*
/*	WATCHDOG *watchdog_create(timeout, action, context)
/*	unsigned timeout;
/*	void	(*action)(WATCHDOG *watchdog, char *context);
/*	char	*context;
/*
/*	void	watchdog_start(watchdog)
/*	WATCHDOG *watchdog;
/*
/*	void	watchdog_stop(watchdog)
/*	WATCHDOG *watchdog;
/*
/*	void	watchdog_destroy(watchdog)
/*	WATCHDOG *watchdog;
/*
/*	void	watchdog_pat()
/* DESCRIPTION
/*	This module implements watchdog timers that are based on ugly
/*	UNIX alarm timers. The module is designed to survive systems
/*	with clocks that jump occasionally.
/*
/*	Watchdog timers can be stacked. Only one watchdog timer can be
/*	active at a time. Only the last created watchdog timer can be
/*	manipulated. Watchdog timers must be destroyed in reverse order
/*	of creation.
/*
/*	watchdog_create() suspends the current watchdog timer, if any,
/*	and instantiates a new watchdog timer.
/*
/*	watchdog_start() starts or restarts the watchdog timer.
/*
/*	watchdog_stop() stops the watchdog timer.
/*
/*	watchdog_destroy() stops the watchdog timer, and resumes the
/*	watchdog timer instance that was suspended by watchdog_create().
/*
/*	watchdog_pat() pats the watchdog, so it stays quiet.
/*
/*	Arguments:
/* .IP timeout
/*	The watchdog time limit. When the watchdog timer runs, the
/*	process must invoke watchdog_start(), watchdog_stop() or
/*	watchdog_destroy() before the time limit is reached.
/* .IP action
/*	A null pointer, or pointer to function that is called when the
/*	watchdog alarm goes off. The default action is to terminate
/*	the process with a fatal error.
/* .IP context
/*	Application context that is passed to the action routine.
/* .IP watchdog
/*	Must be a pointer to the most recently created watchdog instance.
/*	This argument is checked upon each call.
/* BUGS
/*	UNIX alarm timers are not stackable, so there can be at most one
/*	watchdog instance active at any given time.
/* SEE ALSO
/*	msg(3) diagnostics interface
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem, system call failure.
/*	Panics: interface violations.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <unistd.h>
#include <signal.h>
#include <posix_signals.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <watchdog.h>

/* Application-specific. */

 /*
  * Rather than having one timer that goes off when it is too late, we break
  * up the time limit into smaller intervals so that we can deal with clocks
  * that jump occasionally.
  */
#define WATCHDOG_STEPS	3

 /*
  * UNIX alarms are not stackable, but we can save and restore state, so that
  * watchdogs can at least be nested, sort of.
  */
struct WATCHDOG {
    unsigned timeout;			/* our time resolution */
    WATCHDOG_FN action;			/* application routine */
    char   *context;			/* application context */
    int     trip_run;			/* number of successive timeouts */
    WATCHDOG *saved_watchdog;		/* saved state */
    struct sigaction saved_action;	/* saved state */
    unsigned saved_time;		/* saved state */
};

 /*
  * However, only one watchdog instance can be current, and the caller has to
  * restore state before a prior watchdog instance can be manipulated.
  */
static WATCHDOG *watchdog_curr;

/* watchdog_event - handle timeout event */

static void watchdog_event(int unused_sig)
{
    char   *myname = "watchdog_event";
    WATCHDOG *wp;

    /*
     * This routine runs as a signal handler. We should not do anything that
     * could involve memory allocation/deallocation, but exiting without
     * proper explanation would be unacceptable.
     */
    if ((wp = watchdog_curr) == 0)
	msg_panic("%s: no instance", myname);
    if (msg_verbose)
	msg_info("%s: %p %d", myname, (void *) wp, wp->trip_run);
    if (++(wp->trip_run) < WATCHDOG_STEPS) {
	alarm(wp->timeout);
    } else {
	if (wp->action)
	    wp->action(wp, wp->context);
	else
	    msg_fatal("watchdog timeout");
    }
}

/* watchdog_create - create watchdog instance */

WATCHDOG *watchdog_create(unsigned timeout, WATCHDOG_FN action, char *context)
{
    char   *myname = "watchdog_create";
    struct sigaction sig_action;
    WATCHDOG *wp;

    wp = (WATCHDOG *) mymalloc(sizeof(*wp));
    if ((wp->timeout = timeout / WATCHDOG_STEPS) == 0)
	msg_panic("%s: timeout %d is too small", myname, timeout);
    wp->action = action;
    wp->context = context;
    wp->saved_watchdog = watchdog_curr;
    wp->saved_time = alarm(0);
    sigemptyset(&sig_action.sa_mask);
#ifdef SA_RESTART
    sig_action.sa_flags = SA_RESTART;
#else
    sig_action.sa_flags = 0;
#endif
    sig_action.sa_handler = watchdog_event;
    if (sigaction(SIGALRM, &sig_action, &wp->saved_action) < 0)
	msg_fatal("%s: sigaction(SIGALRM): %m", myname);
    if (msg_verbose)
	msg_info("%s: %p %d", myname, (void *) wp, timeout);
    return (watchdog_curr = wp);
}

/* watchdog_destroy - destroy watchdog instance, restore state */

void    watchdog_destroy(WATCHDOG *wp)
{
    char   *myname = "watchdog_destroy";

    watchdog_stop(wp);
    watchdog_curr = wp->saved_watchdog;
    if (sigaction(SIGALRM, &wp->saved_action, (struct sigaction *) 0) < 0)
	msg_fatal("%s: sigaction(SIGALRM): %m", myname);
    if (wp->saved_time)
	alarm(wp->saved_time);
    myfree((char *) wp);
    if (msg_verbose)
	msg_info("%s: %p", myname, (void *) wp);
}

/* watchdog_start - enable watchdog timer */

void    watchdog_start(WATCHDOG *wp)
{
    char   *myname = "watchdog_start";

    if (wp != watchdog_curr)
	msg_panic("%s: wrong watchdog instance", myname);
    wp->trip_run = 0;
    alarm(wp->timeout);
    if (msg_verbose)
	msg_info("%s: %p", myname, (void *) wp);
}

/* watchdog_stop - disable watchdog timer */

void    watchdog_stop(WATCHDOG *wp)
{
    char   *myname = "watchdog_stop";

    if (wp != watchdog_curr)
	msg_panic("%s: wrong watchdog instance", myname);
    alarm(0);
    if (msg_verbose)
	msg_info("%s: %p", myname, (void *) wp);
}

/* watchdog_pat - pat the dog so it stays quiet */

void    watchdog_pat(void)
{
    char   *myname = "watchdog_pat";

    if (watchdog_curr)
	watchdog_curr->trip_run = 0;
    if (msg_verbose)
	msg_info("%s: %p", myname, (void *) watchdog_curr);
}

#ifdef TEST

#include <vstream.h>

main(int unused_argc, char **unused_argv)
{
    WATCHDOG *wp;

    msg_verbose = 1;

    wp = watchdog_create(10, (WATCHDOG_FN) 0, (char *) 0);
    watchdog_start(wp);
    do {
	watchdog_pat();
    } while (VSTREAM_GETCHAR() != VSTREAM_EOF);
    watchdog_destroy(wp);
}

#endif
