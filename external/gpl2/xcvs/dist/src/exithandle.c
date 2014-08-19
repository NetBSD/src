/*
 *    Copyright (c) 2003, Derek Price and Ximbiot <http://ximbiot.com>
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS source
 *    distribution.
 *
 * This is a convenience wrapper for some of the functions in lib/sighandle.c.
 */

#include "cvs.h"

/*
 * Register a handler for all signals.
 */
void
signals_register (RETSIGTYPE (*handler)(int))
{
#ifndef DONT_USE_SIGNALS
#ifdef SIGABRT
	(void) SIG_register (SIGABRT, handler);
#endif
#ifdef SIGHUP
	(void) SIG_register (SIGHUP, handler);
#endif
#ifdef SIGINT
	(void) SIG_register (SIGINT, handler);
#endif
#ifdef SIGQUIT
	(void) SIG_register (SIGQUIT, handler);
#endif
#ifdef SIGPIPE
	(void) signal (SIGPIPE, SIG_IGN);
#endif
#ifdef SIGTERM
	(void) SIG_register (SIGTERM, handler);
#endif
#endif /* !DONT_USE_SIGNALS */
}



/*
 * Register a handler for all signals and exit.
 */
void
cleanup_register (void (*handler) (void))
{
    atexit (handler);

    /* Always calling this function before any other exit handlers guarantees
     * that signals will be blocked by the time the other exit handlers are
     * called.
     *
     * SIG_beginCrSect will be called once for each handler registered via
     * cleanup_register, but there is no unregister routine for atexit() and
     * this seems like minimal overhead.
     *
     * There is no reason to unblock signals again when the exit handlers are
     * done since the program will be exiting anyhow.
     */
    atexit (SIG_beginCrSect);
}
