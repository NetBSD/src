/*
 * Copyright (C) 1993-1996 Wolfgang Solfrank.
 * Copyright (C) 1993-1996 TooLs GmbH.
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
#ifndef	_KGDB_H
#define	_KGDB_H

struct device;

extern int kgdb_probe __P((struct device *, void *, void *));
extern void kgdb_attach __P((struct device *, struct device *, void *));

struct kgdb_allow {
	u_char mask[4];
	u_char match[4];
};
extern struct kgdb_allow kgdballow[];	/* allowed debuggers */
extern int kgdbcount;		/* count of above */

extern int kgdbpanic;

#define	KGDBPORT	1138	/* debugging port */

#ifdef	KGDBUSER
extern char kgdbuser;		/* nonzero, if we want to debug user mode */
#endif

extern void kgdbcopy	__P((void *, void *, int));
extern void kgdbzero	__P((void *, int));
extern int kgdbcmp	__P((void *, void *, int));

extern int kgdbfbyte	__P((unsigned char *));
extern int kgdbsbyte	__P((unsigned char *, int));

struct kgdb_if {
	/* These fields are used by KGDB itself: */
	u_char	myenetaddr[6];	/* to be filled by the driver */
	u_char	myinetaddr[4];
	u_char	hisenetaddr[6];
	u_char	hisinetaddr[4];
	u_char	flags;		/* driver marks KGDB_MYHW here */
	u_char	connect;
	u_char	pkt[1500];
	int	pktlen;
	int	seq;
	int	mtu;
	u_char	ass[1500];
	u_char	assbit[1500/8/8 + 1];
	short	asslen;
	char	gotbuf[32*1024];
	char	*got;
	int	gotlen;
	struct arpcom *arp;
	struct cfdata *cfp;
	char	*name;		/* to be filled by the driver */
	int	port;		/* to be filled by the driver */
	void	(*start)();	/* to be filled by the driver */
	void	(*leave)();	/* to be filled by the driver */
	int	(*receive)();	/* to be filled by the driver */
	void	(*send)();	/* to be filled by the driver */
	/* Additional routines used only if debugging via SLIP: */
	int	(*getc)();	/* to be filled by the driver */
	void	(*putc)();	/* to be filled by the driver */
	/* The rest is solely used by the driver: */
	int	unit;
	int	speed;
	int	fill;
	int	drvflags;
};

/* flags: */
#define	KGDB_MYHW	0x01
#define	KGDB_MYIP	0x02
#define	KGDB_HISHW	0x04
#define	KGDB_HISIP	0x08
#define	KGDB_CONNECTED	0x10

/* connect: */
#define	KGDB_NOIF	0	/* no interface */
#define	KGDB_NO		1	/* no host may connect to kgdb */
#define	KGDB_SAME	2	/* only the previous host may connect */
#define	KGDB_ALL	3	/* any host may connect */
#ifndef	KGDB_DEF
#define	KGDB_DEF	KGDB_ALL /* default to anyone may connect */
#endif

/* Forward declaration to not force <sys/net/if.h> inclusion */
struct ifnet;

/*
 * Interface routines, to be called by kgdb itself
 */
extern void kgdbinet __P((struct kgdb_if *kip));
extern int kgdbifinit __P((struct kgdb_if *kip, int unit));

/*
 * Network interface routines, to be called by network card drivers
 */
extern void kgdbrint __P((struct kgdb_if *kip, struct ifnet *ifp));
extern void kgdbgotpkt __P((struct kgdb_if *kip, char *pkt, int len));
extern __inline void kgdbattach __P((struct kgdb_if *kip, struct arpcom *arp));
extern __inline void kgdbattach(kip, arp)
	struct kgdb_if *kip;
	struct arpcom *arp;
{
	if (!kip->arp)
		kip->arp = arp;
}

/* Routine for SLIP KGDB initialization: (to be called by serial driver) */
extern void kgdb_serial	__P((struct kgdb_if *));

#endif
