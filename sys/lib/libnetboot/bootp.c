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
 * from @(#) Header: bootp.c,v 1.4 93/09/11 03:13:51 leres Exp  (LBL)
 *    $Id: bootp.c,v 1.4 1993/10/18 23:01:13 cgd Exp $
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <errno.h>
#include <string.h>

#include "salibc.h"

#include "netboot.h"
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

static	char vm_zero[4];
static	char vm_rfc1048[4] = VM_RFC1048;
static	char vm_cmu[4] = VM_CMU;

static nvend;
static char *vend[] = {
	vm_zero,			/* try no explicit vendor type first */
	vm_cmu,
	vm_rfc1048			/* try variable format last */
};

/* Local forwards */
static	int bootpsend __P((struct iodesc *, void *, int));
static	int bootprecv __P((struct iodesc*, void *, int));
static	void vend_cmu __P((u_char *));
static	void vend_rfc1048 __P((u_char *, u_int));

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

#ifdef DEBUG
 	if (debug)
 	    printf("bootp: called\n");
#endif
	bp = &wbuf.wbootp;
	pkt = &rbuf.rbootp;
	pkt -= HEADER_SIZE;

	bzero(bp, sizeof(*bp));

	bp->bp_op = BOOTREQUEST;
	bp->bp_htype = 1;		/* 10Mb Ethernet (48 bits) */
	bp->bp_hlen = 6;
	MACPY(d->myea, bp->bp_chaddr);

	d->myport = IPPORT_BOOTPC;
	d->destport = IPPORT_BOOTPS;
	d->destip = INADDR_BROADCAST;

 	while ((have & need) != need) {
#ifdef DEBUG
	        if (debug)
 		    printf("bootp: sendrecv\n");
#endif
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

#ifdef DEBUG
	if (debug)
		printf("bootpsend: called.  we have 0x%x, and need 0x%x\n",
		    have, need);
#endif
	bp = pkt;
	bzero(bp->bp_file, sizeof(bp->bp_file));
	if ((have & BOOT_ROOT) == 0)
		strcpy((char *)bp->bp_file, "root");
	else if ((have & BOOT_SWAP) == 0)
		strcpy((char *)bp->bp_file, "swap");
	bcopy(vend[nvend], bp->bp_vend, sizeof(long));
	/* If we need any vendor info, cycle to a different vendor next time */
	if ((need & ~have) & (BOOT_SMASK | BOOT_GATEIP))
		nvend = (nvend + 1) % (sizeof(vend) / sizeof(vend[0]));
	bp->bp_xid = d->xid;
	bp->bp_secs = (u_long)(getsecs() - bot);
#ifdef DEBUG
	if (debug)
	    printf("bootpsend: calling sendudp\n");
#endif
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
	u_long ul;

#ifdef DEBUG
	if (debug)
	    printf("bootprecv: called\n");
#endif
	bp = (struct bootp *)checkudp(d, pkt, &len);
#ifdef DEBUG
	if (debug)
		printf("bootprecv: checked.  bp = 0x%x, len = %d\n",
		    (unsigned)bp, len);
#endif
	if (bp == NULL || len < sizeof(*bp) || bp->bp_xid != d->xid) {
#ifdef DEBUG
		if (debug) {
			printf("bootprecv: not for us.\n");
			if (bp == NULL)
				printf("bootprecv: bp null\n");
			else {
				if (len < sizeof(*bp))
					printf("bootprecv: expected %d bytes, got %d\n",
					    sizeof(*bp), len);
				if (bp->bp_xid != d->xid)
					printf("bootprecv: expected xid 0x%x, got 0x%x\n",
					    d->xid, bp->bp_xid);
			}
			delay(500);
		}
#endif
		errno = 0;
		return (-1);
	}

#ifdef DEBUG
	if (debug)
	    printf("bootprecv: got one!\n");
	delay(500);
#endif
	/* Pick up our ip address (and natural netmask) */
	if (ntohl(bp->bp_yiaddr.s_addr) != 0 && (have & BOOT_MYIP) == 0) {
		have |= BOOT_MYIP;
		d->myip = ntohl(bp->bp_yiaddr.s_addr);
#ifdef DEBUG
		if (debug)
			printf("our ip address is %s\n", intoa(d->myip));
#endif
		if (IN_CLASSA(d->myip))
			nmask = IN_CLASSA_NET;
		else if (IN_CLASSB(d->myip))
			nmask = IN_CLASSB_NET;
		else
			nmask = IN_CLASSC_NET;
#ifdef DEBUG
		if (debug)
			printf("'native netmask' is %s\n", intoa(nmask));
#endif
	}

	/* Pick up root or swap server address and file spec */
	if (bp->bp_giaddr.s_addr != 0 && bp->bp_file[0] != '\0') {
		if ((have & BOOT_ROOT) == 0) {
			have |= BOOT_ROOT;
			rootip = ntohl(bp->bp_giaddr.s_addr);
#ifdef DEBUG
			if (debug)
				printf("root ip address is %s\n", intoa(rootip));
#endif
			strncpy(rootpath, (char *)bp->bp_file,
			    sizeof(rootpath));
			rootpath[sizeof(rootpath) - 1] = '\0';

			/* Bump xid so next request will be unique */
			++d->xid;
		} else if ((have & BOOT_SWAP) == 0) {
			have |= BOOT_SWAP;
			swapip = ntohl(bp->bp_giaddr.s_addr);
#ifdef DEBUG
			if (debug)
				printf("swap ip address is %s\n", intoa(swapip));
#endif
			strncpy(swappath, (char *)bp->bp_file,
			    sizeof(swappath));
			swappath[sizeof(swappath) - 1] = '\0';

			/* Bump xid so next request will be unique */
			++d->xid;
		}
	}

	/* Suck out vendor info */
	if (bcmp(vm_cmu, bp->bp_vend, sizeof(vm_cmu)) == 0)
		vend_cmu(bp->bp_vend);
	else if (bcmp(vm_rfc1048, bp->bp_vend, sizeof(vm_rfc1048)) == 0)
		vend_rfc1048(bp->bp_vend, sizeof(bp->bp_vend));
	else if (bcmp(vm_zero, bp->bp_vend, sizeof(vm_zero)) != 0) {
		bcopy(bp->bp_vend, &ul, sizeof(ul));
		printf("bootprecv: unknown vendor 0x%x\n", ul);
	}

#ifdef DEBUG
	if (debug)
		delay(500);
#endif

	/* Nothing more to do if we don't know our ip address yet */
	if ((have & BOOT_MYIP) == 0) {
		errno = 0;
		return (-1);
	}

	/* Check subnet mask against net mask; toss if bogus */
	if ((have & BOOT_SMASK) != 0 && (nmask & smask) != nmask) {
#ifdef DEBUG
		if (debug)
			printf("subnet mask (%s) bad\n", intoa(smask));
#endif
		smask = 0;
		have &= ~BOOT_SMASK;
	}

	/* Get subnet (or natural net) mask */
	mask = nmask;
	if ((have & BOOT_SMASK) != 0)
		mask = smask;
#ifdef DEBUG
	if (debug)
		printf("mask: %s\n", intoa(mask));
#endif

	/* We need a gateway if root or swap is on a different net */
	if ((have & BOOT_ROOT) != 0 && !SAMENET(d->myip, rootip, mask)) {
#ifdef DEBUG
		if (debug)
			printf("need gateway for root ip\n");
#endif
		need |= BOOT_GATEIP;
	}
	if ((have & BOOT_SWAP) != 0 && !SAMENET(d->myip, swapip, mask)) {
#ifdef DEBUG
		if (debug)
			printf("need gateway for swap ip\n");
#endif
		need |= BOOT_GATEIP;
	}

	/* Toss gateway if on a different net */
	if ((have & BOOT_GATEIP) != 0 && !SAMENET(d->myip, gateip, mask)) {
#ifdef DEBUG
		if (debug)
			printf("gateway ip (%s) bad\n", intoa(gateip));
#endif
		gateip = 0;
		have &= ~BOOT_GATEIP;
	}
#ifdef DEBUG
	if (debug) {
		printf("gateway ip: %s\n", intoa(gateip));
		delay(500);
	}
#endif
	return (0);
}

static void
vend_cmu(cp)
	u_char *cp;
{
	register struct cmu_vend *vp;

	vp = (struct cmu_vend *)cp;

	if (vp->v_smask.s_addr != 0 && (have & BOOT_SMASK) == 0) {
		have |= BOOT_SMASK;
		smask = ntohl(vp->v_smask.s_addr);
	}
	if (vp->v_dgate.s_addr != 0 && (have & BOOT_GATEIP) == 0) {
		have |= BOOT_GATEIP;
		gateip = ntohl(vp->v_dgate.s_addr);
	}
}

static void
vend_rfc1048(cp, len)
	register u_char *cp;
	u_int len;
{
	register u_char *ep;
	register int size;
	register u_char tag;

	ep = cp + len;

	/* Step over magic cookie */
	cp += sizeof(long);

	while (cp < ep) {
		tag = *cp++;
		size = *cp++;
		if (tag == TAG_END)
			break;

		if (size == sizeof(long)) {
			if (tag == TAG_SUBNET_MASK) {
				have |= BOOT_SMASK;
				bcopy(cp, &smask, sizeof(smask));
				smask = ntohl(smask);
			}
			if (tag == TAG_GATEWAY) {
				have |= BOOT_GATEIP;
				bcopy(cp, &gateip, sizeof(gateip));
				gateip = ntohl(gateip);
			}
		}
		cp += size;
	}
}
