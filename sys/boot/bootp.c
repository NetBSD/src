/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * @(#) $Header: /cvsroot/src/sys/boot/Attic/bootp.c,v 1.1 1993/10/12 06:02:18 glass Exp $ (LBL)
 */

#include <sys/param.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <errno.h>

#include "netboot.h"
#include "config.h"
#include "bootbootp.h"
#include "bootp.h"


/* Machinery used to insure we don't stop until we have everything we need */
#define	BOOT_MYIP	0x01
#define	BOOT_ROOT	0x02
#define	BOOT_SWAP	0x04
#define	BOOT_GATEIP	0x08		/* optional */
#define	BOOT_SMASK	0x10		/* optional */
#define	BOOT_UCRED	0x20		/* not implemented */

static	int have;
static	int need = BOOT_MYIP | BOOT_ROOT | BOOT_SWAP;

/* Local forwards */
static	int bootpsend __P((struct iodesc *, void *, int));
static	int bootprecv __P((struct iodesc*, void *, int));

/* Fetch required bootp infomation */
void
bootp(d)
	register struct iodesc *d;
{
	register struct bootp *bp;
	register void *pkt;
	struct {
		u_char header[HEADER_SIZE];
		struct bootp wbootp;
	} wbuf;
	union {
		u_char buffer[RECV_SIZE];
		struct {
			u_char header[HEADER_SIZE];
			struct bootp xrbootp;
		}xrbuf;
#define rbootp  xrbuf.xrbootp
	} rbuf;

	if (debug)
	    printf("bootp: called\n");
	have = 0;
	bp = &wbuf.wbootp;
	pkt = &rbuf.rbootp;
	pkt = (((char *) pkt) - HEADER_SIZE);

	bzero(bp, sizeof(*bp));

	bp->bp_op = BOOTREQUEST;
	bp->bp_htype = 1;		/* 10Mb Ethernet (48 bits) */
	bp->bp_hlen = 6;
	MACPY(d->myea, bp->bp_chaddr);

	d->myport = IPPORT_BOOTPC;
	d->destport = IPPORT_BOOTPS;
	d->destip = INADDR_BROADCAST;

	while ((have & need) != need) {
	        if (debug)
		    printf("bootp: sendrecv\n");
		(void)sendrecv(d, bootpsend, bp, sizeof(*bp),
		    bootprecv, pkt, RECV_SIZE);
	}
}

/* Transmit a bootp request */
static int
bootpsend(d, pkt, len)
	register struct iodesc *d;
	register void *pkt;
	register int len;
{
	register struct bootp *bp;

	if (debug)
	    printf("bootpsend: called\n");
	bp = pkt;
	bzero(bp->bp_file, sizeof(bp->bp_file));
	if ((have & BOOT_ROOT) == 0)
		strcpy((char *)bp->bp_file, "root");
	else if ((have & BOOT_SWAP) == 0)
		strcpy((char *)bp->bp_file, "swap");
	bp->bp_xid = d->xid;
	bp->bp_secs = (u_long)(getsecs() - bot);
	if (debug)
	    printf("bootpsend: calling sendudp\n");
	return (sendudp(d, pkt, len));
}

/* Returns 0 if this is the packet we're waiting for else -1 (and errno == 0) */
static int
bootprecv(d, pkt, len)
	register struct iodesc *d;
	register void *pkt;
	int len;
{
	register struct bootp *bp;
	register struct cmu_vend *vp;

	if (debug)
	    printf("bootprecv: called\n");
	bp = (struct bootp *)checkudp(d, pkt, &len);
	if (bp == NULL || len < sizeof(*bp)) {
		errno = 0;
		return (-1);
	}

	if (bp->bp_xid != d->xid) {
		errno = 0;
		return (-1);
	}
	vp = (struct cmu_vend *)bp->bp_vend;
	if (bcmp(VM_CMU, vp->v_magic, sizeof(vp->v_magic)) != 0) {
	        printf("bootprecv: not cmu magic\n");
		vp = NULL;
	}

	/* Suck out all we can */
	if (bp->bp_yiaddr.s_addr != 0 && (have & BOOT_MYIP) == 0) {
		have |= BOOT_MYIP;
		d->myip = bp->bp_yiaddr.s_addr;
		if (IN_CLASSA(d->myip))
			nmask = IN_CLASSA_NET;
		else if (IN_CLASSB(d->myip))
			nmask = IN_CLASSB_NET;
		else
			nmask = IN_CLASSC_NET;
	}
	if (vp && vp->v_smask.s_addr != 0 && (have & BOOT_SMASK) == 0) {
		have |= BOOT_SMASK;
		smask = vp->v_smask.s_addr;
	}
	if (vp && vp->v_dgate.s_addr != 0 && (have & BOOT_GATEIP) == 0) {
		have |= BOOT_GATEIP;
		gateip = vp->v_dgate.s_addr;
	}
	if (bp->bp_giaddr.s_addr != 0 && bp->bp_file[0] != '\0') {
		if ((have & BOOT_ROOT) == 0) {
			have |= BOOT_ROOT;
			rootip = bp->bp_giaddr.s_addr;
			strncpy(rootpath, (char *)bp->bp_file,
			    sizeof(rootpath));
			rootpath[sizeof(rootpath) - 1] = '\0';

			/* Bump xid so next request will be unique */
			++d->xid;
		} else if ((have & BOOT_SWAP) == 0) {
			have |= BOOT_SWAP;
			swapip = bp->bp_giaddr.s_addr;
			strncpy(swappath, (char *)bp->bp_file,
			    sizeof(swappath));
			swappath[sizeof(swappath) - 1] = '\0';

			/* Bump xid so next request will be unique */
			++d->xid;
		}
	}

	/* Done if we don't know our ip address yet */
	if ((have & BOOT_MYIP) == 0) {
		errno = 0;
		return (-1);
	}

	/* Check subnet mask against net mask; toss if bogus */
	if ((have & BOOT_SMASK) != 0 && (nmask & smask) != nmask) {
		smask = 0;
		have &= ~BOOT_SMASK;
	}

	/* Get subnet (or net) mask */
	mask = nmask;
	if ((have & BOOT_SMASK) != 0)
		mask = smask;

	/* We need a gateway if root or swap is on a different net */
	if ((have & BOOT_ROOT) != 0 && !SAMENET(d->myip, rootip, mask))
		need |= BOOT_GATEIP;
	if ((have & BOOT_SWAP) != 0 && !SAMENET(d->myip, swapip, mask))
		need |= BOOT_GATEIP;

	/* Toss gateway if on a different net */
	if ((have & BOOT_GATEIP) != 0 && !SAMENET(d->myip, gateip, mask)) {
		gateip = 0;
		have &= ~BOOT_GATEIP;
	}
	return (0);
}
