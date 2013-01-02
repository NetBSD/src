/*	$NetBSD: msg_rate_delay.c,v 1.1.1.1 2013/01/02 18:59:13 tron Exp $	*/

/*++
/* NAME
/*	msg_rate_delay 3
/* SUMMARY
/*	diagnostic interface
/* SYNOPSIS
/*	#include <msg.h>
/*
/*	void	msg_rate_delay(stamp, delay, log_fn, fmt, ...)
/*	time_t	*stamp;
/*	int	delay;
/*	void	(*log_fn)(const char *fmt, ...);
/*	const char *fmt;
/* DESCRIPTION
/*	msg_rate_delay() produces log output at a reduced rate: no
/*	more than one message per 'delay' seconds. It discards log
/*	output that would violate the output rate policy.
/*
/*	This is typically used to log errors accessing a cache with
/*	high-frequency access but low-value information, to avoid
/*	spamming the logfile with the same kind of message.
/*
/*	Arguments:
/* .IP stamp
/*	Time stamp of last log output; specify a zero time stamp
/*	on the first call.  This is an input-output parameter.
/*	This parameter is ignored when verbose logging is enabled
/*	or when the delay value is zero.
/* .IP delay
/*	The minimum time between log outputs; specify zero to log
/*	all output for debugging purposes.  This parameter is ignored
/*	when verbose logging is enabled.
/* .IP log_fn
/*	The function that produces log output. Typically, this will
/*	be msg_info() or msg_warn().
/* .IP fmt
/*	Format string as used with msg(3) routines.
/* SEE ALSO
/*	msg(3) diagnostics interface
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem.
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
#include <time.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <events.h>

/* SLMs. */

#define STR(x) vstring_str(x)

/* msg_rate_delay - rate-limit message logging */

void    msg_rate_delay(time_t *stamp, int delay,
		               void (*log_fn) (const char *,...),
		               const char *fmt,...)
{
    const char *myname = "msg_rate_delay";
    static time_t saved_event_time;
    time_t  now;
    VSTRING *buf;
    va_list ap;

    /*
     * Sanity check.
     */
    if (delay < 0)
	msg_panic("%s: bad message rate delay: %d", myname, delay);

    /*
     * This function may be called frequently. Avoid an unnecessary syscall
     * if possible. Deal with the possibility that a program does not use the
     * events(3) engine, so that event_time() always produces the same
     * result.
     */
    if (msg_verbose == 0 && delay > 0) {
	if (saved_event_time == 0)
	    now = saved_event_time = event_time();
	else if ((now = event_time()) == saved_event_time)
	    now = time((time_t *) 0);

	/*
	 * Don't log if time is too early.
	 */
	if (*stamp + delay > now)
	    return;
	*stamp = now;
    }

    /*
     * OK to log. This is a low-rate event, so we can afford some overhead.
     */
    buf = vstring_alloc(100);
    va_start(ap, fmt);
    vstring_vsprintf(buf, fmt, ap);
    va_end(ap);
    log_fn("%s", STR(buf));
    vstring_free(buf);
}

#ifdef TEST

 /*
  * Proof-of-concept test program: log messages but skip messages during a
  * two-second gap.
  */
#include <unistd.h>

int     main(int argc, char **argv)
{
    int     n;
    time_t  stamp = 0;

    for (n = 0; n < 6; n++) {
	msg_rate_delay(&stamp, 2, msg_info, "text here %d", n);
	sleep(1);
    }
    return (0);
}

#endif
