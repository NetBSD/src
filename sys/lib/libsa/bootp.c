/*	$NetBSD: bootp.c,v 1.3 1995/02/20 11:04:04 mycroft Exp $	*/

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
 * @(#) Header: bootp.c,v 1.4 93/09/11 03:13:51 leres Exp  (LBL)
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <string.h>

#include "stand.h"
#include "net.h"
#include "netif.h"
#include "bootp.h"

static n_long	nmask, smask;

static time_t	bot;

static	char vm_rfc1048[4] = VM_RFC1048;
static	char vm_cmu[4] = VM_CMU;

/* Local forwards */
static	size_t bootpsend __P((struct iodesc *, void *, size_t));
static	size_t bootprecv __P((struct iodesc *, void *, size_t, time_t));
static	void vend_cmu __P((u_char *));
static	void vend_rfc1048 __P((u_char *, u_int));

/* Fetch required bootp infomation */
void
bootp(sock)
	int sock;
{
	struct iodesc *d;
	register struct bootp *bp;
	struct {
		u_char header[HEADER_SIZE];
		struct bootp wbootp;
	} wbuf;
	struct {
		u_char header[HEADER_SIZE];
		struct bootp rbootp;
	} rbuf;

#ifdef BOOTP_DEBUG
 	if (debug)
		printf("bootp: socket=%d\n", sock);
#endif
	if (!bot)
		bot = getsecs();
	
	if (!(d = socktodesc(sock))) {
		printf("bootp: bad socket. %d\n", sock);
		return;
	}
#ifdef BOOTP_DEBUG
 	if (debug)
		printf("bootp: d=%x\n", (u_int)d);
#endif
	bp = &wbuf.wbootp;
	bzero(bp, sizeof(*bp));

	bp->bp_op = BOOTREQUEST;
	bp->bp_htype = 1;		/* 10Mb Ethernet (48 bits) */
	bp->bp_hlen = 6;
	MACPY(d->myea, bp->bp_chaddr);

	d->xid = 0;
	d->myip = myip;
	d->myport = IPPORT_BOOTPC;
	d->destip = INADDR_BROADCAST;
	d->destport = IPPORT_BOOTPS;

	(void)sendrecv(d,
	    bootpsend, bp, sizeof(*bp),
	    bootprecv, &rbuf.rbootp, sizeof(rbuf.rbootp));
}

/* Transmit a bootp request */
static size_t
bootpsend(d, pkt, len)
	register struct iodesc *d;
	register void *pkt;
	register size_t len;
{
	register struct bootp *bp;

#ifdef BOOTP_DEBUG
	if (debug)
		printf("bootpsend: d=%x called.\n", (u_int)d);
#endif

	bp = pkt;
	bzero(bp->bp_file, sizeof(bp->bp_file));

	bcopy(vm_rfc1048, bp->bp_vend, sizeof(long));
	bp->bp_xid = d->xid;
	bp->bp_secs = (u_long)(getsecs() - bot);

#ifdef BOOTP_DEBUG
	if (debug)
		printf("bootpsend: calling sendudp\n");
#endif

	return (sendudp(d, pkt, len));
}

/* Returns 0 if this is the packet we're waiting for else -1 (and errno == 0) */
static size_t
bootprecv(d, pkt, len, tleft)
	register struct iodesc *d;
	register void *pkt;
	register size_t len;
	time_t tleft;
{
	register struct bootp *bp;

#ifdef BOOTP_DEBUG
	if (debug)
		printf("bootprecv: called\n");
#endif

	len = readudp(d, pkt, len, tleft);
	if (len == -1 || len < sizeof(struct bootp))
		goto bad;

	bp = (struct bootp *)pkt;
	NTOHL(bp->bp_xid);

#ifdef BOOTP_DEBUG
	if (debug)
		printf("bootprecv: checked.  bp = 0x%x, len = %d\n",
		    (unsigned)bp, len);
#endif
	if (bp->bp_xid != d->xid) {
#ifdef BOOTP_DEBUG
		if (debug) {
			printf("bootprecv: expected xid 0x%x, got 0x%x\n",
			    d->xid, bp->bp_xid);
		}
#endif
		goto bad;
	}

	/* Bump xid so next request will be unique. */
	++d->xid;
	
#ifdef BOOTP_DEBUG
	if (debug)
		printf("bootprecv: got one!\n");
#endif

	/* Pick up our ip address (and natural netmask) */
	myip = d->myip = ntohl(bp->bp_yiaddr.s_addr);
#ifdef BOOTP_DEBUG
	if (debug)
		printf("our ip address is %s\n", intoa(d->myip));
#endif
	if (IN_CLASSA(d->myip))
		nmask = IN_CLASSA_NET;
	else if (IN_CLASSB(d->myip))
		nmask = IN_CLASSB_NET;
	else
		nmask = IN_CLASSC_NET;
#ifdef BOOTP_DEBUG
	if (debug)
		printf("'native netmask' is %s\n", intoa(nmask));
#endif

	/* Pick up root or swap server address and file spec. */
	if (bp->bp_siaddr.s_addr != 0)
		rootip = ntohl(bp->bp_siaddr.s_addr);
	if (bp->bp_file[0] != '\0') {
		strncpy(bootfile, (char *)bp->bp_file, sizeof(bootfile));
		bootfile[sizeof(bootfile) - 1] = '\0';
	}

	/* Suck out vendor info */
	if (bcmp(vm_cmu, bp->bp_vend, sizeof(vm_cmu)) == 0)
		vend_cmu(bp->bp_vend);
	else if (bcmp(vm_rfc1048, bp->bp_vend, sizeof(vm_rfc1048)) == 0)
		vend_rfc1048(bp->bp_vend, sizeof(bp->bp_vend));
	else
		printf("bootprecv: unknown vendor 0x%x\n", (int)bp->bp_vend);

	/* Check subnet mask against net mask; toss if bogus */
	if ((nmask & smask) != nmask) {
#ifdef BOOTP_DEBUG
		if (debug)
			printf("subnet mask (%s) bad\n", intoa(smask));
#endif
		smask = 0;
	}

	/* Get subnet (or natural net) mask */
	mask = nmask;
	if (smask)
		mask = smask;
#ifdef BOOTP_DEBUG
	if (debug)
		printf("mask: %s\n", intoa(mask));
#endif

	/* We need a gateway if root or swap is on a different net */
	if (!SAMENET(d->myip, rootip, mask)) {
#ifdef BOOTP_DEBUG
		if (debug)
			printf("need gateway for root ip\n");
#endif
	}

	if (!SAMENET(d->myip, swapip, mask)) {
#ifdef BOOTP_DEBUG
		if (debug)
			printf("need gateway for swap ip\n");
#endif
	}

	/* Toss gateway if on a different net */
	if (!SAMENET(d->myip, gateip, mask)) {
#ifdef BOOTP_DEBUG
		if (debug)
			printf("gateway ip (%s) bad\n", intoa(gateip));
#endif
		gateip = 0;
	}

	return (0);

bad:
	errno = 0;
	return (-1);
}

static void
vend_cmu(cp)
	u_char *cp;
{
	register struct cmu_vend *vp;

#ifdef BOOTP_DEBUG
	if (debug)
		printf("vend_cmu bootp info.\n");
#endif
	vp = (struct cmu_vend *)cp;

	if (vp->v_smask.s_addr != 0) {
		smask = ntohl(vp->v_smask.s_addr);
	}
	if (vp->v_dgate.s_addr != 0) {
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

#ifdef BOOTP_DEBUG
	if (debug)
		printf("vend_rfc1048 bootp info. len=%d\n", len);
#endif
	ep = cp + len;

	/* Step over magic cookie */
	cp += sizeof(long);

	while (cp < ep) {
		tag = *cp++;
		size = *cp++;
		if (tag == TAG_END)
			break;

		if (tag == TAG_SUBNET_MASK) {
			bcopy(cp, &smask, sizeof(smask));
			smask = ntohl(smask);
		}
		if (tag == TAG_GATEWAY) {
			bcopy(cp, &gateip, sizeof(gateip));
			gateip = ntohl(gateip);
		}
		if (tag == TAG_SWAPSERVER) {
			bcopy(cp, &swapip, sizeof(swapip));
			swapip = ntohl(swapip);
		}
		if (tag == TAG_DOMAIN_SERVER) {
			bcopy(cp, &nameip, sizeof(nameip));
			nameip = ntohl(nameip);
		}
		if (tag == TAG_ROOTPATH) {
			strncpy(rootpath, cp, sizeof(rootpath));
			rootpath[size] = '\0';
		}
		if (tag == TAG_HOSTNAME) {
			strncpy(hostname, cp, sizeof(hostname));
			hostname[size] = '\0';
		}
		if (tag == TAG_DOMAINNAME) {
			strncpy(domainname, cp, sizeof(domainname));
			domainname[size] = '\0';
		}
		cp += size;
	}
}
