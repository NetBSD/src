/*	$NetBSD: if_le.c,v 1.12.42.1 2017/08/28 17:51:54 skrll Exp $ */
/*
 * Copyright (c) 1997, 1999 Ludd, University of Lule}, Sweden.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Standalone routine for MicroVAX LANCE chip. 
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <../include/sid.h>
#include <../include/rpb.h>

#include <lib/libsa/netif.h>
#include <lib/libsa/stand.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/am7990reg.h>

#include "vaxstand.h"

/*
 * Buffer sizes.
 */
#define TLEN    1
#define NTBUF   (1 << TLEN)
#define RLEN    3
#define NRBUF   (1 << RLEN)
#define BUFSIZE 1518

#define	QW_ALLOC(x)	(((uintptr_t)alloc((x) + 7) + 7) & ~7)

static int le_get(struct iodesc *, void *, size_t, saseconds_t);
static int le_put(struct iodesc *, void *, size_t);
static void copyout(void *from, int dest, int len);
static void copyin(int src, void *to, int len);

struct netif_driver le_driver = {
	0, 0, 0, 0, le_get, le_put,
};

/*
 * Init block & buffer descriptors according to DEC system
 * specification documentation.
 */
struct initblock {
	short	ib_mode;
	char	ib_padr[6]; /* Ethernet address */
	int	ib_ladrf1;
	int	ib_ladrf2;
	int	ib_rdr; /* Receive address */
	int	ib_tdr; /* Transmit address */
} *initblock = NULL;

struct nireg {
	volatile u_short ni_rdp;       /* data port */
	volatile short ni_pad0;
	volatile short ni_rap;       /* register select port */
} *nireg;


volatile struct	buffdesc {
	int	bd_adrflg;
	short	bd_bcnt;
	short	bd_mcnt;
} *rdesc, *tdesc;

static	int addoff, kopiera = 0;

/* Flags in the address field */
#define	BR_OWN	0x80000000
#define	BR_ERR	0x40000000
#define	BR_FRAM	0x20000000
#define	BR_OFLO	0x10000000
#define	BR_CRC	0x08000000
#define	BR_BUFF	0x04000000
#define	BR_STP	0x02000000
#define	BR_ENP	0x01000000

#define	BT_OWN	0x80000000
#define	BT_ERR	0x40000000
#define	BT_MORE	0x10000000
#define	BT_ONE	0x08000000
#define	BT_DEF	0x04000000
#define	BT_STP	0x02000000
#define	BT_ENP	0x01000000

int	next_rdesc, next_tdesc;

#define	LEWRCSR(port, val) { \
	nireg->ni_rap = (port); \
	nireg->ni_rdp = (val); \
}

#define	LERDCSR(port) \
	(nireg->ni_rap = port, nireg->ni_rdp)
 
int
leopen(struct open_file *f, int adapt, int ctlr, int unit, int part)
{
	int i, *ea;
	volatile int to = 100000;
	u_char eaddr[6];

	next_rdesc = next_tdesc = 0;

	if (vax_boardtype == VAX_BTYP_650 &&
	    ((vax_siedata >> 8) & 0xff) == VAX_SIE_KA640) {
		kopiera = 1;
		ea = (void *)0x20084200;
		nireg = (void *)0x20084400;
	} else {
		*(int *)0x20080014 = 0; /* Be sure we do DMA in low 16MB */
		ea = (void *)0x20090000; /* XXX ethernetaddress */
		nireg = (void *)0x200e0000;
	}
	if (askname == 0) /* Override if autoboot */
		nireg = (void *)bootrpb.csrphy;
	else /* Tell kernel from where we booted */
		bootrpb.csrphy = (int)nireg;

	if (vax_boardtype == VAX_BTYP_43)
		addoff = 0x28000000;
	else
		addoff = 0;
igen:
	LEWRCSR(LE_CSR0, LE_C0_STOP);
	while (to--)
		;

	for (i = 0; i < 6; i++)
		eaddr[i] = ea[i] & 0377;

	if (initblock == NULL) {
		initblock = (struct initblock *)
			(QW_ALLOC(sizeof(struct initblock)) + addoff);
		initblock->ib_mode = LE_MODE_NORMAL;
		memcpy(initblock->ib_padr, eaddr, 6);
		initblock->ib_ladrf1 = 0;
		initblock->ib_ladrf2 = 0;

		rdesc = (struct buffdesc *)
			(QW_ALLOC(sizeof(struct buffdesc) * NRBUF) + addoff);
		initblock->ib_rdr = (RLEN << 29) | (int)rdesc;
		if (kopiera)
			initblock->ib_rdr -= (int)initblock;
		tdesc = (struct buffdesc *)
			(QW_ALLOC(sizeof(struct buffdesc) * NTBUF) + addoff);
		initblock->ib_tdr = (TLEN << 29) | (int)tdesc;
		if (kopiera)
			initblock->ib_tdr -= (int)initblock;
		if (kopiera)
			copyout(initblock, 0, sizeof(struct initblock));

		for (i = 0; i < NRBUF; i++) {
			rdesc[i].bd_adrflg = QW_ALLOC(BUFSIZE) | BR_OWN;
			if (kopiera)
				rdesc[i].bd_adrflg -= (int)initblock;
			rdesc[i].bd_bcnt = -BUFSIZE;
			rdesc[i].bd_mcnt = 0;
		}
		if (kopiera)
			copyout((void *)rdesc, (int)rdesc - (int)initblock,
			    sizeof(struct buffdesc) * NRBUF);

		for (i = 0; i < NTBUF; i++) {
			tdesc[i].bd_adrflg = QW_ALLOC(BUFSIZE);
			if (kopiera)
				tdesc[i].bd_adrflg -= (int)initblock;
			tdesc[i].bd_bcnt = 0xf000;
			tdesc[i].bd_mcnt = 0;
		}
		if (kopiera)
			copyout((void *)tdesc, (int)tdesc - (int)initblock,
			    sizeof(struct buffdesc) * NTBUF);
	}

	if (kopiera) {
		LEWRCSR(LE_CSR1, 0);
		LEWRCSR(LE_CSR2, 0);
	} else {
		LEWRCSR(LE_CSR1, (int)initblock & 0xffff);
		LEWRCSR(LE_CSR2, ((int)initblock >> 16) & 0xff);
	}

	LEWRCSR(LE_CSR0, LE_C0_INIT);

	to = 100000;
	while (to--) {
		if (LERDCSR(LE_CSR0) & LE_C0_IDON)
			break;
		if (LERDCSR(LE_CSR0) & LE_C0_ERR) {
			printf("lance init error: csr0 %x\n", LERDCSR(LE_CSR0));
			goto igen;
		}
	}

	LEWRCSR(LE_CSR0, LE_C0_INEA | LE_C0_STRT | LE_C0_IDON);

	net_devinit(f, &le_driver, eaddr);
	return 0;
}

int
le_get(struct iodesc *desc, void *pkt, size_t maxlen, saseconds_t timeout)
{
	int csr, len;
	volatile int to = 100000 * timeout;

retry:
	if (to-- == 0)
		return 0;

	csr = LERDCSR(LE_CSR0);
	LEWRCSR(LE_CSR0, csr & (LE_C0_BABL|LE_C0_MISS|LE_C0_MERR|LE_C0_RINT));

	if (kopiera)
		copyin((int)&rdesc[next_rdesc] - (int)initblock,
		    (void *)&rdesc[next_rdesc], sizeof(struct buffdesc));
	if (rdesc[next_rdesc].bd_adrflg & BR_OWN)
		goto retry;

        if (rdesc[next_rdesc].bd_adrflg & BR_ERR)
                len = 0;
        else {
		if ((len = rdesc[next_rdesc].bd_mcnt - 4) > maxlen)
			len = maxlen;

		if (kopiera)
			copyin((rdesc[next_rdesc].bd_adrflg&0xffffff),
			    pkt, len);
		else
			memcpy(pkt,
			    (char *)(rdesc[next_rdesc].bd_adrflg&0xffffff) +
			    addoff, len);
	}

	rdesc[next_rdesc].bd_mcnt = 0;
	rdesc[next_rdesc].bd_adrflg |= BR_OWN;
	if (kopiera)
		copyout((void *)&rdesc[next_rdesc], (int)&rdesc[next_rdesc] - 
		    (int)initblock, sizeof(struct buffdesc));
	if (++next_rdesc >= NRBUF)
		next_rdesc = 0;


	if (len == 0)
		goto retry;
	return len;
}

int
le_put(struct iodesc *desc, void *pkt, size_t len)
{
	volatile int to = 100000;
	int csr;

retry:
	if (--to == 0)
		return -1;

	csr = LERDCSR(LE_CSR0);
	LEWRCSR(LE_CSR0, csr & (LE_C0_MISS|LE_C0_CERR|LE_C0_TINT));

	if (kopiera)
		copyin((int)&tdesc[next_tdesc] - (int)initblock,
		    (void *)&tdesc[next_tdesc], sizeof(struct buffdesc));
	if (tdesc[next_tdesc].bd_adrflg & BT_OWN)
		goto retry;

	if (kopiera)
		copyout(pkt, (tdesc[next_tdesc].bd_adrflg & 0xffffff), len);
	else
		memcpy((char *)(tdesc[next_tdesc].bd_adrflg & 0xffffff) +
		    addoff, pkt, len);
	tdesc[next_tdesc].bd_bcnt =
	    (len < ETHER_MIN_LEN ? -ETHER_MIN_LEN : -len);
	tdesc[next_tdesc].bd_mcnt = 0;
	tdesc[next_tdesc].bd_adrflg |= BT_OWN | BT_STP | BT_ENP;
	if (kopiera)
		copyout((void *)&tdesc[next_tdesc], (int)&tdesc[next_tdesc] - 
		    (int)initblock, sizeof(struct buffdesc));

	LEWRCSR(LE_CSR0, LE_C0_TDMD);

	to = 100000;
	while (((LERDCSR(LE_CSR0) & LE_C0_TINT) == 0) && --to)
		;

	LEWRCSR(LE_CSR0, LE_C0_TINT);
	if (++next_tdesc >= NTBUF)
		next_tdesc = 0;

	if (to)
		return len;

	return -1;
}

int
leclose(struct open_file *f)
{
	LEWRCSR(LE_CSR0, LE_C0_STOP);

	return 0;
}

void
copyout(void *f, int dest, int len)
{
	short *from = f;
	short *toaddr;

	toaddr = (short *)0x20120000 + dest;

	while (len > 0) {
		*toaddr = *from++;
		toaddr += 2;
		len -= 2;
	}
}

void
copyin(int src, void *f, int len)
{
	short *to = f;
	short *fromaddr;

	fromaddr = (short *)0x20120000 + src;

	while (len > 0) {
		*to++ = *fromaddr;
		fromaddr += 2;
		len -= 2;
	}
}
