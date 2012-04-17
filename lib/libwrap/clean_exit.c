/*	$NetBSD: clean_exit.c,v 1.4.64.1 2012/04/17 00:05:34 yamt Exp $	*/

 /*
  * clean_exit() cleans up and terminates the program. It should be called
  * instead of exit() when for some reason the real network daemon will not or
  * cannot be run. Reason: in the case of a datagram-oriented service we must
  * discard the not-yet received data from the client. Otherwise, inetd will
  * see the same datagram again and again, and go into a loop.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#) clean_exit.c 1.4 94/12/28 17:42:19";
#else
__RCSID("$NetBSD: clean_exit.c,v 1.4.64.1 2012/04/17 00:05:34 yamt Exp $");
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tcpd.h"

/* clean_exit - clean up and exit */

void
clean_exit(struct request_info *request)
{

    /*
     * In case of unconnected protocols we must eat up the not-yet received
     * data or inetd will loop.
     */

    if (request->sink)
	request->sink(request->fd);

    /*
     * Be kind to the inetd. We already reported the problem via the syslogd,
     * and there is no need for additional garbage in the logfile.
     */

    sleep(5);
    exit(0);
}
