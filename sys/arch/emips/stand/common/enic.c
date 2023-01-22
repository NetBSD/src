/*	$NetBSD: enic.c,v 1.5 2023/01/22 21:36:12 andvar Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* --------------------------------------------------------------------------
 *
 * Module:
 *
 *    enic.c
 *
 * Purpose:
 *
 *    Driver for the Microsoft eNIC (eMIPS system) Ethernet
 *
 * Author:
 *    A. Forin (sandrof)
 *
 * References:
 *    Internal Microsoft specifications, file eNIC_Design.docx titled
 *    "eNIC: a simple Ethernet" Revision 4/30/99
 *
 *    Giano simulation module, file Peripherals\enic.cpp
 *
 *    Various other drivers I wrote for said hardware
 * --------------------------------------------------------------------------
 */

#include <sys/param.h>
#include <sys/types.h>

#include <net/if_ether.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>
#include <lib/libkern/libkern.h>


#include <machine/emipsreg.h>

#include "start.h"
#include "common.h"

#define the_enic ((struct _Enic *)ETHERNET_DEFAULT_ADDRESS)

/* forward declarations */
static int  enicprobe (struct netif *, void *);
static int  enicmatch (struct netif *, void *);
static void enicinit (struct iodesc *, void *);
static int  enicget (struct iodesc *, void *, size_t, saseconds_t);
static int  enicput (struct iodesc *, void *, size_t);
static void enicend (struct netif *);

#ifdef NET_DEBUG
static void dump_packet(void *, int);
#endif

/* BUGBUG do we have this? */
#define kvtophys(_v_)  ((paddr_t)(_v_) & ~0x80000000)

#define rpostone(_r_,_p_,_s_) \
    (_r_)->SizeAndFlags = ES_F_RECV | (_s_); \
    (_r_)->BufferAddressHi32 = 0; \
    (_r_)->BufferAddressLo32 = _p_;
#define tpostone(_r_,_p_,_s_) \
    (_r_)->SizeAndFlags = ES_F_XMIT | (_s_); \
    (_r_)->BufferAddressHi32 = 0; \
    (_r_)->BufferAddressLo32 = _p_;


/* Send a packet
 */
static int
enic_putpkt(struct _Enic *regs, void *buf, int bytes)
{
    paddr_t phys = kvtophys(buf);

    tpostone(regs,phys,bytes);

    /* poll till done? */
    //printf("\tenic: sent %d at %x\n",bytes,phys);
    return bytes;
}

/* Get a packet
 */
static int
enic_getpkt(struct _Enic *regs, void *buf, int bytes, int timeo)
{
    paddr_t phys;
    unsigned int isr, saf, hi, lo, fl;

    phys = kvtophys(buf);
    rpostone(regs,phys,bytes);

    //printf("\tenic: recv %d at %x\n",bytes,phys);

    /* poll till we get some */
    timeo += getsecs();

    for (;;) {

        /* anything there? */
        isr = regs->Control;

        if (isr & EC_ERROR) {
            printf("enic: internal error %x\n", isr);
            regs->Control = EC_RESET;
            break;
        }
		
		//printf("isr %x ",isr);

        if ((isr & (EC_DONE|EC_OF_EMPTY)) == EC_DONE) {

            /* beware, order matters */
            saf = regs->SizeAndFlags;
            hi = regs->BufferAddressHi32; /* BUGBUG 64bit */
            lo = regs->BufferAddressLo32; /* this pops the fifo */
	    __USE(hi);

            fl = saf & (ES_F_MASK &~ ES_F_DONE);

            if (fl == ES_F_RECV)
            {
                /* and we are done? */
                if (phys == lo)
                    return saf & ES_S_MASK;
            } else if (fl == ES_F_XMIT)
            {
                ;/* nothing */
            } else if (fl != ES_F_CMD)
            {
                printf("enic: invalid saf=x%x (lo=%x, hi=%x)\n", saf, lo, hi);
            }
        }

        if (getsecs() >= timeo) {
            //printf("enic: timeout\n");
            regs->Control = EC_RESET;
            break;
        }
    }

    return 0;
}

/* 
 */
static int enic_getmac(struct _Enic *regs, uint8_t *mac)
{
    uint8_t buffer[8];
    paddr_t phys = kvtophys(&buffer[0]);
    int i;

    regs->Control = EC_RESET;
    Delay(1);
    regs->Control = regs->Control & (~EC_RXDIS);

    buffer[0] = ENIC_CMD_GET_ADDRESS;

	//printf("%x:%x:%x:%x:%x:%x\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);

    regs->SizeAndFlags = (sizeof buffer) | ES_F_CMD;
    regs->BufferAddressHi32 = 0;
    regs->BufferAddressLo32 = phys; /* go! */

    for (i = 0; i < 100; i++) {
        Delay(100);
        if (0 == (regs->Control & EC_OF_EMPTY))
            break;
    }
    if (i == 100)
        return 0;

	//printf("%x:%x:%x:%x:%x:%x\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);

    memcpy(mac,buffer,6);
    return 1;
}

/* Exported interface
 */
int
enic_present(int unit)
{
    if ((unit != 0) || (the_enic->Tag != PMTTAG_ETHERNET))
	return 0;

    return 1;
}

extern int try_bootp;

extern struct netif_stats       enicstats[];
struct netif_dif enicifs[] = {
/*	dif_unit	dif_nsel	dif_stats		dif_private	*/
{	0,			1,			&enicstats[0],	the_enic,		},
};
#define NENICIFS (sizeof(enicifs) / sizeof(enicifs[0]))
struct netif_stats enicstats[NENICIFS];

struct netif_driver enic_netif_driver = {
	"enic",				/* netif_bname */
	enicmatch,			/* netif_match */
	enicprobe,			/* netif_probe */
	enicinit,			/* netif_init */
	enicget,			/* netif_get */
	enicput,			/* netif_put */
	enicend,			/* netif_end */
	enicifs,			/* netif_ifs */
	NENICIFS			/* netif_nifs */
};

static int
enicmatch(struct netif *nif, void *machdep_hint)
{
	return (1);
}

/* NB: We refuse anything but unit==0
 */
static int
enicprobe(struct netif *nif, void *machdep_hint)
{
    int unit = nif->nif_unit;
#ifdef NET_DEBUG
	printf("enic%d: probe\n", unit);
#endif
	return (enic_present(unit) ? 0 : 1);
}

static void
enicinit(struct iodesc *desc, void *machdep_hint)
{
#ifdef NET_DEBUG
    struct netif *nif = (struct netif *)desc->io_netif;
    int unit = nif->nif_driver->netif_ifs->dif_unit;
	printf("enic%d: init %s\n", unit, machdep_hint);
#endif

    /*
     * Yes we want DHCP and this is our MAC
     */
	try_bootp = 1;
    enic_getmac(the_enic,desc->myea);
	desc->xid = 0xfe63d095;
}


static int
enicput(struct iodesc *desc, void *pkt,	size_t len)
{
#ifdef NET_DEBUG
	if (debug)
		dump_packet(pkt,len);
#endif

    return enic_putpkt(the_enic,pkt,len);
}


int
enicget(struct iodesc *desc, void *pkt, size_t len, saseconds_t timeout)
{
#ifdef NET_DEBUG
	printf("enicget: %lx %lx\n",len,timeout);
#endif
    return enic_getpkt(the_enic,pkt,len,timeout);
}


static void
enicend(struct netif *nif)
{
    /* BUGBUG stop it in reset? */
}

#ifdef NET_DEBUG
static void dump_packet(void *pkt, int len)
{
	struct ether_header *eh = (struct ether_header *)pkt;
	struct ip *ih = (struct ip *)(eh + 1);

	printf("ether_dhost = %s\n", ether_sprintf(eh->ether_dhost));
	printf("ether_shost = %s\n", ether_sprintf(eh->ether_shost));
	printf("ether_type = 0x%x\n", ntohs(eh->ether_type));

	if (ntohs(eh->ether_type) == 0x0800) {
		printf("ip packet version %d\n", ih->ip_v);
		printf("source ip: 0x%x\n", ih->ip_src.s_addr);
		printf("dest ip: 0x%x\n", ih->ip_dst.s_addr);

	}
}
#endif
