/*	$NetBSD: ipkdb.h,v 1.4 2000/03/22 20:58:29 ws Exp $	*/

/*
 * Copyright (C) 1993-2000 Wolfgang Solfrank.
 * Copyright (C) 1993-2000 TooLs GmbH.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef	_IPKDB_H
#define	_IPKDB_H

extern int ipkdbpanic;

#define	IPKDBPORT	1138	/* debugging port */

struct ipkdb_if {
	/* These fields are used by IPKDB itself: */
	u_int8_t	myenetaddr[6];	/* to be filled by the driver */
	u_int8_t	myinetaddr[4];
	u_int8_t	hisenetaddr[6];
	u_int8_t	hisinetaddr[4];
	u_int16_t	hisport;
	u_char		pkt[1500];
	int		pktlen;
	u_int32_t	seq;
	u_int32_t	id;
	int		mtu;
	u_char		ass[1500];
	u_int8_t	assbit[1500/8/8 + 1];
	u_int16_t	asslen;
	u_int8_t	flags;		/* driver marks IPKDB_MYHW here */
	/* Data from here on is to be filled by the driver */
	char		*name;
	void		*port;
	void		(*start) __P((struct ipkdb_if *));
	void		(*leave) __P((struct ipkdb_if *));
	int		(*receive) __P((struct ipkdb_if *, u_char *, int));
	void		(*send) __P((struct ipkdb_if *, u_char *, int));
};

/* flags: */
#define	IPKDB_MYHW	0x01
#define	IPKDB_MYIP	0x02
#define	IPKDB_HISHW	0x04
#define	IPKDB_HISIP	0x08
#define	IPKDB_CONNECTED	0x10

/*
 * Interface routines, to be called by machine dependent code.
 */
extern void ipkdb_init __P((void));
extern void ipkdb_connect __P((int));
extern void ipkdb_panic __P((void));
extern int ipkdbcmds __P((void));
/* Return values from ipkdbcmds: */
#define	IPKDB_CMD_RUN	0
#define	IPKDB_CMD_STEP	1
#define	IPKDB_CMD_EXIT	2

/* To be called by udp_input on receipt of a possible debugging packet */
struct in_addr;
struct mbuf;
extern int checkipkdb __P((struct in_addr *, u_short, u_short,
			struct mbuf *, int, int));

/*
 * Interface routines, to be called by ipkdb itself.
 */
extern int ipkdbifinit __P((struct ipkdb_if *));

/*
 * Utilities (used to avoid calling system routines during debugging).
 */
extern void ipkdbcopy __P((void *, void *, int));
extern void ipkdbzero __P((void *, int));
extern int ipkdbcmp __P((void *, void *, int));

/*
 * Machine dependent routines for IPKDB.
 */
extern void ipkdbinit __P((void));
extern void ipkdb_trap __P((void));
extern int ipkdb_poll __P((void));

extern int ipkdbif_init __P((struct ipkdb_if *));

extern int ipkdbfbyte __P((u_char *));
extern int ipkdbsbyte __P((u_char *, int));

#endif	/* _IPKDB_H */
