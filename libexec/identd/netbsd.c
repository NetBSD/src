/*	$NetBSD: netbsd.c,v 1.18 2003/06/26 16:23:53 christos Exp $	*/

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

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <nlist.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>


#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <sys/socketvar.h>

#include <sys/file.h>

#include <sys/sysctl.h>

#include <fcntl.h>

#include <sys/user.h>

#include <sys/wait.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <netinet/in_pcb.h>

#include <netinet/tcp.h>
#include <netinet/ip_var.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <arpa/inet.h>

#include "identd.h"
#include "error.h"


int k_open()
{
  return 0;
}


/*
** Return the user number for the connection owner
*/
int k_getuid(faddr, fport, laddr, lport, uid
#ifdef ALLOW_FORMAT
			 , pid, cmd, cmd_and_args
#endif
			 )
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
  int *uid;
#ifdef ALLOW_FORMAT
  int *pid;
  char **cmd, **cmd_and_args;
#endif
{
  int mib[4];
  uid_t myuid = -1;
  size_t uidlen;
  struct sysctl_tcp_ident_args args;
  int rv;
  
  mib[0] = CTL_NET;
  mib[1] = PF_INET;
  mib[2] = IPPROTO_TCP;
  mib[3] = TCPCTL_IDENT;

  args.raddr = *faddr;
  args.rport = fport;
  args.laddr = *laddr;
  args.lport = lport;

  uidlen = sizeof(myuid);
  
    if ((rv = sysctl(mib, 4, &myuid, &uidlen, &args,sizeof(args))) < 0)
    {
      ERROR1("k_getuid: sysctl 1: %s", strerror(errno));
      return -1;
    }
   
  *uid = myuid;
  return 0;
}
