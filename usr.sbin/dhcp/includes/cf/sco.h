/* sco.h

   System dependencies for SCO ODT 3.0...

   Based on changes contributed by Gerald Rosenberg. */

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

#include <syslog.h>
#include <sys/types.h>

/* Basic Integer Types not defined in SCO headers... */

typedef char int8_t;
typedef short int16_t;
typedef long int32_t; 

typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t; 
typedef unsigned long u_int32_t;

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <setjmp.h>
#include <limits.h>

extern int h_errno;

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_arp.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

/* XXX dunno if this is required for SCO... */
/*
 * Definitions for IP type of service (ip_tos)
 */
#define IPTOS_LOWDELAY          0x10
#define IPTOS_THROUGHPUT        0x08
#define IPTOS_RELIABILITY       0x04
/*      IPTOS_LOWCOST           0x02 XXX */

/* SCO doesn't have /var/run. */
#ifndef _PATH_DHCPD_CONF
#define _PATH_DHCPD_CONF	"/etc/dhcpd.conf"
#endif
#ifndef _PATH_DHCPD_PID
#define _PATH_DHCPD_PID 	"/etc/dhcpd.pid"
#endif
#ifndef _PATH_DHCLIENT_PID
#define _PATH_DHCLIENT_PID  "/etc/dhclient.pid"
#endif
#ifndef _PATH_DHCRELAY_PID
#define _PATH_DHCRELAY_PID  "/etc/dhcrelay.pid"
#endif
#ifndef _PATH_DHCPD_DB
#define _PATH_DHCPD_DB      "/etc/dhcpd.leases"
#endif
#ifndef _PATH_DHCLIENT_DB
#define _PATH_DHCLIENT_DB   "/etc/dhclient.leases"
#endif


#if !defined (INADDR_LOOPBACK)
#define INADDR_LOOPBACK	((u_int32_t)0x7f000001)
#endif

/* Varargs stuff: use stdarg.h instead ... */
#include <stdarg.h>
#define VA_DOTDOTDOT ...
#define VA_start(list, last) va_start (list, last)
#define va_dcl

/* SCO doesn't support limited sprintfs. */
#define vsnprintf(buf, size, fmt, list) vsprintf (buf, fmt, list)
#define NO_SNPRINTF

/* By default, use BSD Socket API for receiving and sending packets.
   This actually works pretty well on Solaris, which doesn't censor
   the all-ones broadcast address. */
#if defined (USE_DEFAULT_NETWORK)
# define USE_SOCKETS
#endif

#define EOL	'\n'
#define VOIDPTR	void *

/* socklen_t */
typedef int socklen_t;

/*
 * Time stuff...
 *
 * Definitions for an ISC DHCPD system that uses time_t
 * to represent time internally as opposed to, for example,  struct timeval.)
 */

#include <time.h>
#include <sys/time.h>

#define TIME time_t
#define GET_TIME(x)	time ((x))

#ifdef NEED_PRAND_CONF
const char *cmds[] = {
	"/bin/ps -ef 2>&1",
	"/etc/arp -n -a 2>&1",
	"/usr/bin/netstat -an 2>&1",
	"/bin/df  2>&1",
	"/usr/bin/uptime  2>&1",
	"/usr/bin/netstat -s 2>&1",
	"/usr/bin/vmstat  2>&1",
	"/usr/bin/w  2>&1",
	NULL
};

const char *dirs[] = {
	"/tmp",
	"/usr/tmp",
	".",
	"/",
	"/var/spool",
	"/var/adm",
	"/dev",
	NULL
};

const char *files[] = {
	NULL
};
#endif /* NEED_PRAND_CONF */
