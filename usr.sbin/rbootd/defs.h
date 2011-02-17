/*	$NetBSD: defs.h,v 1.9.2.1 2011/02/17 12:00:59 bouyer Exp $	*/

/*
 * Copyright (c) 1988, 1992 The University of Utah and the Center
 *	for Software Science (CSS).
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Center for Software Science of the University of Utah Computer
 * Science Department.  CSS requests users of this software to return
 * to css-dist@cs.utah.edu any improvements that they make and grant
 * CSS redistribution rights.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)defs.h	8.1 (Berkeley) 6/4/93
 *
 * From: Utah Hdr: defs.h 3.1 92/07/06
 * Author: Jeff Forys, University of Utah CSS
 */

#include "rmp.h"
#include "rmp_var.h"

/*
**  Common #define's and external variables.  All other files should
**  include this.
*/

/*
 *  This may be defined in <sys/param.h>, if not, it's defined here.
 */
#ifndef	MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN 64
#endif

/*
 *  SIGUSR1 and SIGUSR2 are defined in <signal.h> for 4.3BSD systems.
 */
#ifndef SIGUSR1
#define	SIGUSR1 SIGEMT
#endif
#ifndef SIGUSR2
#define	SIGUSR2 SIGFPE
#endif

/*
 *  These can be faster & more efficient than strcmp()/strncmp()...
 */
#define	STREQN(s1,s2)		((*s1 == *s2) && (strcmp(s1,s2) == 0))
#define	STRNEQN(s1,s2,n)	((*s1 == *s2) && (strncmp(s1,s2,n) == 0))

/*
 *  Configuration file limitations.
 */
#define	C_MAXFILE	10		/* max number of boot-able files */
#define	C_LINELEN	1024		/* max length of line */

/*
 *  Direction of packet (used as argument to DispPkt).
 */
#define	DIR_RCVD	0
#define	DIR_SENT	1
#define	DIR_NONE	2

/*
 *  These need not be functions, so...
 */
#define	FreeStr(str)	free(str)
#define	FreeClient(cli)	free(cli)
#define	GenSessID()	(++SessionID ? SessionID: ++SessionID)

/*
 *  Converting an Ethernet address to a string is done in many routines.
 *  Using `rmp.hp_hdr.saddr' works because this field is *never* changed;
 *  it will *always* contain the source address of the packet.
 */
#define	EnetStr(rptr)	GetEtherAddr(&(rptr)->rmp.hp_hdr.saddr[0])

/*
 *  Every machine we can boot will have one of these allocated for it
 *  (unless there are no restrictions on who we can boot).
 */
typedef struct client_s {
	u_int8_t		addr[RMP_ADDRLEN];	/* addr of machine */
	char			*files[C_MAXFILE];	/* boot-able files */
	struct client_s		*next;			/* ptr to next */
} CLIENT;

/*
 *  Every active connection has one of these allocated for it.
 */
typedef struct rmpconn_s {
	struct rmp_packet	rmp;			/* RMP packet */
	int			rmplen;			/* length of packet */
	struct timeval		tstamp;			/* last time active */
	int			bootfd;			/* open boot file */
	struct rmpconn_s	*next;			/* ptr to next */
} RMPCONN;

/*
 *  All these variables are defined in "conf.c".
 */
extern	char	MyHost[MAXHOSTNAMELEN+1]; /* this hosts' name */
extern	int	DebugFlg;		/* set true if debugging */
extern	int	BootAny;		/* set true if we can boot anyone */

extern	const char *ConfigFile;		/* configuration file */
extern	const char *DfltConfig;		/* default configuration file */
extern	const char *DbgFile;		/* debug output file */
extern	const char *PidFile;		/* file containing pid of server */
extern	const char *BootDir;		/* directory w/boot files */

extern	FILE	*DbgFp;			/* debug file pointer */
extern	char	*IntfName;		/* interface we are attached to */

extern	u_int16_t SessionID;		/* generated session ID */

extern	char	*BootFiles[];		/* list of boot files */

extern	CLIENT	*Clients;		/* list of addrs we'll accept */
extern	RMPCONN	*RmpConns;		/* list of active connections */

extern	u_int8_t RmpMcastAddr[];	/* RMP multicast address */

void	 AddConn __P((RMPCONN *));
int	 BootDone __P((RMPCONN *));
void	 BpfClose __P((void));
char	*BpfGetIntfName __P((char **));
int	 BpfOpen __P((void));
int	 BpfRead __P((RMPCONN *, int));
int	 BpfWrite __P((RMPCONN *));
void	 DebugOff __P((int));
void	 DebugOn __P((int));
void	 DispPkt __P((RMPCONN *, int));
void	 DoTimeout __P((void));
void	 DspFlnm __P((u_int, char *));
void	 Exit __P((int));
CLIENT	*FindClient __P((RMPCONN *));
RMPCONN	*FindConn __P((RMPCONN *));
void	 FreeClients __P((void));
void	 FreeConn __P((RMPCONN *));
void	 FreeConns __P((void));
int	 GetBootFiles __P((void));
char	*GetEtherAddr __P((u_int8_t *));
CLIENT	*NewClient __P((u_int8_t *));
RMPCONN	*NewConn __P((RMPCONN *));
char	*NewStr __P((char *));
u_int8_t *ParseAddr __P((char *));
int	 ParseConfig __P((void));
void	 ProcessPacket __P((RMPCONN *, CLIENT *));
void	 ReConfig __P((int));
void	 RemoveConn __P((RMPCONN *));
int	 SendBootRepl __P((struct rmp_packet *, RMPCONN *, char *[]));
int	 SendFileNo __P((struct rmp_packet *, RMPCONN *, char *[]));
int	 SendPacket __P((RMPCONN *));
int	 SendReadRepl __P((RMPCONN *));
int	 SendServerID __P((RMPCONN *));
