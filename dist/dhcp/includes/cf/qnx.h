/* qnx.h

   System dependencies for QNX...  */

/*
 * Copyright (c) 1996-1999 Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about the Internet Software Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <setjmp.h>
#include <limits.h>
#include <syslog.h>
#include <sys/select.h>

#include <sys/wait.h>
#include <signal.h>

#ifdef __QNXNTO__
#include <sys/param.h>
#endif

#include <netdb.h>
extern int h_errno;

#include <net/if.h>
#ifndef __QNXNTO__
# define INADDR_LOOPBACK ((u_long)0x7f000001)
#endif

/* Varargs stuff... */
#include <stdarg.h>
#define VA_DOTDOTDOT ...
#define va_dcl
#define VA_start(list, last) va_start (list, last)

#ifndef _PATH_DHCPD_PID
#define _PATH_DHCPD_PID	"/etc/dhcpd.pid"
#endif
#ifndef _PATH_DHCLIENT_PID
#define _PATH_DHCLIENT_PID "/etc/dhclient.pid"
#endif
#ifndef _PATH_DHCRELAY_PID
#define _PATH_DHCRELAY_PID "/etc/dhcrelay.pid"
#endif

#define EOL	'\n'
#define VOIDPTR void *

/* Time stuff... */
#include <sys/time.h>
#define TIME time_t
#define GET_TIME(x)	time ((x))
#define TIME_DIFF(high, low)	 	(*(high) - *(low))
#define SET_TIME(x, y)	(*(x) = (y))
#define ADD_TIME(d, s1, s2) (*(d) = *(s1) + *(s2))
#define SET_MAX_TIME(x)	(*(x) = INT_MAX)

#ifndef __QNXNTO__
typedef unsigned char	u_int8_t;
typedef unsigned short	u_int16_t;
typedef unsigned long	u_int32_t;
typedef signed short	int16_t;
typedef signed long	int32_t;
#endif

#ifdef __QNXNTO__
typedef int socklen_t;
#endif

#define strcasecmp( s1, s2 )			stricmp( s1, s2 )
#define strncasecmp( s1, s2, n )		strnicmp( s1, s2, n )
#define random()				rand()

#define HAVE_SA_LEN
#define BROKEN_TM_GMT
#define USE_SOCKETS
#undef AF_LINK

#ifndef __QNXNTO__
# define NO_SNPRINTF
# define vsnprintf( buf, size, fmt, list )	vsprintf( buf, fbuf, list )
#endif

#ifdef __QNXNTO__
# define GET_HOST_ID_MISSING
#endif

/*
    NOTE: to get the routing of the 255.255.255.255 broadcasts to work
    under QNX, you need to issue the following command before starting
    the daemon:

    	route add -interface 255.255.255.0 <hostname>

    where <hostname> is replaced by the hostname or IP number of the
    machine that dhcpd is running on.
*/

#ifndef __QNXNTO__
# if defined (NSUPDATE)
# error NSUPDATE is not supported on QNX at this time!!
# endif
#endif


#ifdef NEED_PRAND_CONF
#ifndef HAVE_DEV_RANDOM
/* You should find and install the /dev/random driver */
 # define HAVE_DEV_RANDOM 1
 #endif /* HAVE_DEV_RANDOM */

const char *cmds[] = {
        "/bin/ps -a 2>&1",
	"/bin/sin 2>&1",
        "/sbin/arp -an 2>&1",
        "/bin/netstat -an 2>&1",
        "/bin/df  2>&1",
	"/bin/sin fds 2>&1",
        "/bin/netstat -s 2>&1",
	"/bin/sin memory 2>&1",
        NULL
};

const char *dirs[] = {
        "/tmp",
        ".",
        "/",
        "/var/spool",
        "/dev",
        "/var/spool/mail",
        "/home",
        NULL
};

const char *files[] = {
        "/proc/ipstats",
        "/proc/dumper",
        "/proc/self/as",
        "/var/log/messages",
        NULL
};
#endif /* NEED_PRAND_CONF */

