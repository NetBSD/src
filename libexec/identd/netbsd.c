/*	$NetBSD: netbsd.c,v 1.19 2003/06/26 17:31:12 christos Exp $	*/

/*
** netbsd.c		Low level kernel access functions for NetBSD
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Last update: 15 July 1998
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include "identd.h"
#include "error.h"


int k_open()
{
  return 0;
}


/*
** Return the user number for the connection owner
*/
int k_getuid(
	struct in_addr *faddr,
	int fport,
	struct in_addr *laddr,
	int lport,
	int *uid
#ifdef ALLOW_FORMAT
	, int *pid
	, char **cmd
	, char **cmd_and_args
#endif
)
{
	int mib[8];
	uid_t myuid = -1;
	size_t uidlen = sizeof(myuid);
	int rv;

	mib[0] = CTL_NET;
	mib[1] = PF_INET;
	mib[2] = IPPROTO_TCP;
	mib[3] = TCPCTL_IDENT;
	mib[4] = (int)faddr->s_addr;
	mib[5] = fport;
	mib[6] = (int)laddr->s_addr;
	mib[7] = lport;

	if ((rv = sysctl(mib, sizeof(mib), &myuid, &uidlen, NULL, 0)) < 0) {
		ERROR1("k_getuid: sysctl 1: %s", strerror(errno));
		return -1;
	}

	*uid = myuid;
	return 0;
}
