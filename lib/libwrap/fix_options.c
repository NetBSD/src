 /*
  * Routine to disable IP-level socket options. This code was taken from 4.4BSD
  * rlogind source, but all mistakes in it are my fault.
  *
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) fix_options.c 1.3 94/12/28 17:42:22";
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>

#include "tcpd.h"

/* fix_options - get rid of IP-level socket options */

fix_options(request)
struct request_info *request;
{
#ifdef IP_OPTIONS
    unsigned char optbuf[BUFSIZ / 3], *cp;
    char    lbuf[BUFSIZ], *lp;
    int     optsize = sizeof(optbuf), ipproto;
    struct protoent *ip;
    int     fd = request->fd;

    if ((ip = getprotobyname("ip")) != 0)
	ipproto = ip->p_proto;
    else
	ipproto = IPPROTO_IP;

    if (getsockopt(fd, ipproto, IP_OPTIONS, (char *) optbuf, &optsize) == 0
	&& optsize != 0) {
	lp = lbuf;
	for (cp = optbuf; optsize > 0; cp++, optsize--, lp += 3)
	    sprintf(lp, " %2.2x", *cp);
	syslog(LOG_NOTICE,
	       "connect from %s with IP options (ignored):%s",
	       eval_client(request), lbuf);
	if (setsockopt(fd, ipproto, IP_OPTIONS, (char *) 0, optsize) != 0) {
	    syslog(LOG_ERR, "setsockopt IP_OPTIONS NULL: %m");
	    clean_exit(request);
	}
    }
#endif
}
