/*++
/* NAME
/*	master_sig 3
/* SUMMARY
/*	Postfix master - signal processing
/* SYNOPSIS
/*	#include "master.h"
/*
/*	int	master_gotsighup;
/*	int	master_gotsigchld;
/*
/*	int	master_sigsetup()
/* DESCRIPTION
/*	This module implements the master process signal handling interface.
/*
/*	master_gotsighup (master_gotsigchld) is set to SIGHUP (SIGCHLD)
/*	when the process receives a hangup (child death) signal.
/*
/*	master_sigsetup() enables processing of hangup and child death signals.
/*	Receipt of SIGINT, SIGQUIT, SIGSEGV, SIGILL, or SIGTERM
/*	is interpreted as a request for termination.  Child processes are
/*	notified of the master\'s demise by sending them a SIGTERM signal.
/* DIAGNOSTICS
/* BUGS
/*	Need a way to register cleanup actions.
/* SEE ALSO
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

/* System libraries. */

#include <sys_defs.h>
#include <signal.h>
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <posix_signals.h>

/* Application-specific. */

#include "master.h"

#ifdef USE_SIG_RETURN
#include <sys/syscall.h>
#endif

/* Local stuff. */

#ifdef USE_SIG_PIPE
#include <errno.h>
#include <fcntl.h>
#include <iostuff.h>

int     master_sig_pipe[2];

#define SIG_PIPE_WRITE_FD master_sig_pipe[1]
#define SIG_PIPE_READ_FD master_sig_pipe[0]
#endif

int     master_gotsigchld;
int     master_gotsighup;

/* master_sighup - register arrival of hangup signal */

static void master_sighup(int sig)
{

    /*
     * WARNING WARNING WARNING.
     * 
     * This code runs at unpredictable moments, as a signal handler. Don't put
     * any code here other than for setting a global flag.
     */
    master_gotsighup = sig;
}

/* master_sigchld - register arrival of child death signal */

#ifdef USE_SIG_RETURN

static void master_sigchld(int sig, int code, struct sigcontext * scp)
{

    /*
     * WARNING WARNING WARNING.
     * 
     * This code runs at unpredictable moments, as a signal handler. Don't put
     * any code here other than for setting a global flag, or code that is
     * intended to be run within a signal handler.
     */
    master_gotsigchld = sig;
    if (scp != NULL && scp->sc_syscall == SYS_select) {
	scp->sc_syscall_action = SIG_RETURN;
#ifndef SA_RESTART
    } else if (scp != NULL) {
	scp->sc_syscall_action = SIG_RESTART;
#endif
    }
}

#else

#ifdef USE_SIG_PIPE

/* master_sigchld - force wakeup from select() */

static void master_sigchld(int sig)
{
    if (write(SIG_PIPE_WRITE_FD, "", 1) != 1)
	msg_warn("write to SIG_PIPE_WRITE_FD failed: %m");
}

/* master_sig_event - called upon return from select() */

static void master_sig_event(int unused_event, char *unused_context)
{
    char    c[1];

    while (read(SIG_PIPE_READ_FD, c, 1) > 0)
	 /* void */ ;
    master_gotsigchld = 1;
}

#else

static void master_sigchld(int sig)
{

    /*
     * WARNING WARNING WARNING.
     * 
     * This code runs at unpredictable moments, as a signal handler. Don't put
     * any code here other than for setting a global flag.
     */
    master_gotsigchld = sig;
}

#endif
#endif

/* master_sigdeath - die, women and children first */

static void master_sigdeath(int sig)
{
    char   *myname = "master_sigsetup";
    struct sigaction action;
    pid_t   pid = getpid();

    /*
     * XXX We're running from a signal handler, and really should not call
     * any msg() routines at all, but it would be even worse to silently
     * terminate without informing the sysadmin.
     */
    msg_info("terminating on signal %d", sig);

    /*
     * Terminate all processes in our process group, except ourselves.
     */
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = SIG_IGN;
    if (sigaction(SIGTERM, &action, (struct sigaction *) 0) < 0)
	msg_fatal("%s: sigaction: %m", myname);
    if (kill(-pid, SIGTERM) < 0)
	msg_fatal("%s: kill process group: %m", myname);

    /*
     * Deliver the signal to ourselves and clean up. XXX We're running as a
     * signal handler and really should not be doing complicated things...
     */
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = SIG_DFL;
    if (sigaction(sig, &action, (struct sigaction *) 0) < 0)
	msg_fatal("%s: sigaction: %m", myname);
    if (kill(pid, sig) < 0)
	msg_fatal("%s: kill myself: %m", myname);
}

/* master_sigsetup - set up signal handlers */

void    master_sigsetup(void)
{
    char   *myname = "master_sigsetup";
    struct sigaction action;
    static int sigs[] = {
	SIGINT, SIGQUIT, SIGILL, SIGBUS, SIGSEGV, SIGTERM,
    };
    unsigned i;

    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    /*
     * Prepare to kill our children when we receive any of the above signals.
     */
    action.sa_handler = master_sigdeath;
    for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
	if (sigaction(sigs[i], &action, (struct sigaction *) 0) < 0)
	    msg_fatal("%s: sigaction(%d): %m", myname, sigs[i]);

#ifdef USE_SIG_PIPE
    if (pipe(master_sig_pipe))
	msg_fatal("pipe: %m");
    non_blocking(SIG_PIPE_WRITE_FD, NON_BLOCKING);
    non_blocking(SIG_PIPE_READ_FD, NON_BLOCKING);
    close_on_exec(SIG_PIPE_WRITE_FD, CLOSE_ON_EXEC);
    close_on_exec(SIG_PIPE_READ_FD, CLOSE_ON_EXEC);
    event_enable_read(SIG_PIPE_READ_FD, master_sig_event, (char *) 0);
#endif

    /*
     * Intercept SIGHUP (re-read config file) and SIGCHLD (child exit).
     */
#ifdef SA_RESTART
    action.sa_flags |= SA_RESTART;
#endif
    action.sa_handler = master_sighup;
    if (sigaction(SIGHUP, &action, (struct sigaction *) 0) < 0)
	msg_fatal("%s: sigaction(%d): %m", myname, SIGHUP);

    action.sa_flags |= SA_NOCLDSTOP;
    action.sa_handler = master_sigchld;
    if (sigaction(SIGCHLD, &action, (struct sigaction *) 0) < 0)
	msg_fatal("%s: sigaction(%d): %m", myname, SIGCHLD);
}
