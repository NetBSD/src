/*
 * Copyright (c) 1993 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
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
 * $Header: /cvsroot/src/sys/arch/sun3/netboot/Attic/le_poll.c,v 1.1 1993/10/12 05:23:31 glass Exp $
 */

#include <sys/param.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "netboot/netboot.h"
#include "netboot/netif.h"
#include "config.h"

#include "machine/obio.h"
#include "../dev/if_lereg.h"
#include "../dev/if_le_subr.h"

int le_debug = 1;

int le_probe();
int le_match();
void le_init();
int le_get();
int le_put();
void le_end();

struct netif_stats le_stats;

struct netif le_netif = {
    "le",			/* netif_bname */
    0,				/* netif_unit */
    0,				/* netif_exhausted */
    le_match,			/* netif_match */
    le_probe,			/* netif_probe */
    le_init,			/* netif_init */
    le_get,			/* netif_get */
    le_put,			/* netif_put */
    le_end,			/* netif_end */
    &le_stats
};

struct le_configuration {
    unsigned int obio_addr;
    int used;
} le_config[] = {
    {OBIO_AMD_ETHER, 0}
};

int nle_config = sizeof(le_config)/(sizeof(le_config[0]));

#define LE_UNIT le_netif.netif_unit

struct {
	struct	lereg1 *sc_r1;	/* LANCE registers */
	struct	lereg2 *sc_r2;	/* RAM */
	int next_rmd;
	int next_tmd;
} le_softc; 

int le_match(machdep_hint, unitp)
     void *machdep_hint;
     int *unitp;
{
    char *name;
    int i, val = 0;
    
    name = machdep_hint;
    if (name && !strncmp(le_netif.netif_bname, name,2))
	val += 10;
    for (i = 0; i < nle_config; i++) {
	if (le_config[i].used) continue;
	*unitp = i;
	if (le_debug)
	    printf("le%d: le_match --> %d\n", i, val+1);
	le_config[i].used++;
	return val+1;
    }
    if (le_debug)
	printf("le%d: le_match --> 0\n", i);
    return 0;
}
      
int le_probe(machdep_hint)
     void *machdep_hint;
{
    /* the set unit is the current unit */
    if (le_debug)
	printf("le%d: le_probe called\n", LE_UNIT);
    return 0;
}

void le_sanity_check(str)
     char *str;
{
    struct lereg1 *ler1 = le_softc.sc_r1;
    struct lereg2 *ler2 = le_softc.sc_r2;
    unsigned int a;
    int i;

    for (i = 0; i < LERBUF; i++) {
	a = LANCE_ADDR(&ler2->ler2_rbuf[i]);
	if ((ler2->ler2_rmd[i].rmd0 != (a & LE_ADDR_LOW_MASK)) ||
	    (ler2->ler2_rmd[i].rmd1_hadr != (a >> 16))) {
	    printf("le%d: ler2_rmd[%d] addrs bad\n", LE_UNIT, i);
	    printf("le%d: string: %s\n", LE_UNIT, str);
	    panic("addrs trashed\n");
	}
    }
    for (i = 0; i < LETBUF; i++) {
	a = LANCE_ADDR(&ler2->ler2_tbuf[i]);
	if ((ler2->ler2_tmd[i].tmd0 != (a & LE_ADDR_LOW_MASK)) ||
	    (ler2->ler2_tmd[i].tmd1_hadr != (a >> 16))) {
	    printf("le%d: ler2_tmd[%d] addrs bad\n", LE_UNIT, i);
	    printf("le%d: string: %s\n", LE_UNIT, str);
	    panic("addrs trashed\n");
	}
    }
    
}

void le_mem_summary()
{
    struct lereg1 *ler1 = le_softc.sc_r1;
    struct lereg2 *ler2 = le_softc.sc_r2;
    int i;

    printf("le%d: obio addr = %x\n", LE_UNIT, ler1);
    printf("le%d: dvma addr = %x\n", LE_UNIT, ler2);
    
#if 0
    ler1->ler1_rap = LE_CSR0;
    ler1->ler1_rdp = LE_STOP;
    printf("le%d: csr0 = %x\n", LE_UNIT, ler1->ler1_rdp);
    ler1->ler1_rap = LE_CSR1;
    printf("le%d: csr1 = %x\n", LE_UNIT, ler1->ler1_rdp);
    ler1->ler1_rap = LE_CSR2;
    printf("le%d: csr2 = %x\n", LE_UNIT, ler1->ler1_rdp);
    ler1->ler1_rap = LE_CSR3;
    printf("le%d: csr3 = %x\n", LE_UNIT, ler1->ler1_rdp);
#endif
    printf("le%d: ladrf0 = %x\n", LE_UNIT, ler2->ler2_ladrf0);
    printf("le%d: ladrf1 = %x\n", LE_UNIT, ler2->ler2_ladrf1);
    printf("le%d: ler2_rdra = %x\n", LE_UNIT, ler2->ler2_rdra);
    printf("le%d: ler2_rlen = %x\n", LE_UNIT, ler2->ler2_rlen);
    printf("le%d: ler2_tdra = %x\n", LE_UNIT, ler2->ler2_tdra);	
    printf("le%d: ler2_tlen = %x\n", LE_UNIT, ler2->ler2_tlen);

    for (i = 0; i < LERBUF; i++) {
	printf("le%d: ler2_rmd[%d].rmd0 (ladr) = %x\n", LE_UNIT, i,
	       ler2->ler2_rmd[i].rmd0);
	printf("le%d: ler2_rmd[%d].rmd1_bits = %x\n", LE_UNIT, i,
	       ler2->ler2_rmd[i].rmd1_bits);
	printf("le%d: ler2_rmd[%d].rmd1_hadr = %x\n", LE_UNIT, i,
	       ler2->ler2_rmd[i].rmd1_hadr);
	printf("le%d: ler2_rmd[%d].rmd2 (-bcnt) = %x\n", LE_UNIT, i,
	       ler2->ler2_rmd[i].rmd2);
	printf("le%d: ler2_rmd[%d].rmd3 (mcnt) = %x\n", LE_UNIT, i,
	       ler2->ler2_rmd[i].rmd3);
	printf("le%d: ler2_rbuf[%d] addr = %x\n", LE_UNIT, i,
	       &ler2->ler2_rbuf[i]);
    }
    for (i = 0; i < LETBUF; i++) {
	printf("le%d: ler2_tmd[%d].tmd0 = %x\n", LE_UNIT, i,
	       ler2->ler2_tmd[i].tmd0);
	printf("le%d: ler2_tmd[%d].tmd1_bits = %x\n", LE_UNIT, i,
	       ler2->ler2_tmd[i].tmd1_bits);
	printf("le%d: ler2_tmd[%d].tmd1_hadr = %x\n", LE_UNIT, i,
	       ler2->ler2_tmd[i].tmd1_hadr);
	printf("le%d: ler2_tmd[%d].tmd2 (bcnt) = %x\n", LE_UNIT, i,
	       ler2->ler2_tmd[i].tmd2);
	printf("le%d: ler2_tmd[%d].tmd3 = %x\n", LE_UNIT, i,
	       ler2->ler2_tmd[i].tmd3);
	printf("le%d: ler2_tbuf[%d] addr = %x\n", LE_UNIT, i,
	       &ler2->ler2_tbuf[i]);
    }
}


void le_error(str, ler1)
     char *str;
     struct lereg1 *ler1;
{
    /* ler1->ler1_rap = LE_CSRO done in caller */
    if (ler1->ler1_rdp & LE_BABL)
	panic("le%d: been babbling, found by '%s'\n", LE_UNIT, str);
    if (ler1->ler1_rdp & LE_CERR) {
	le_stats.collision_error++;
	ler1->ler1_rdp = LE_CERR;
    }
    if (ler1->ler1_rdp & LE_MISS) {
	le_stats.missed++;
	ler1->ler1_rdp = LE_MISS;
    }
    if (ler1->ler1_rdp & LE_MERR) { 
	printf("le%d: memory error in '%s'\n", LE_UNIT, str);
	le_mem_summary();
	panic("bye");
    }
    
}

void le_reset(myea)
     u_char *myea;
{
    struct lereg1 *ler1 = le_softc.sc_r1;
    struct lereg2 *ler2 = le_softc.sc_r2;
    unsigned int a;
    int timo = 100000, stat, i;

    if (le_debug)
	printf("le%d: le_reset called\n", LE_UNIT);
    ler1->ler1_rap = LE_CSR0;
    ler1->ler1_rdp = LE_STOP;	/* do nothing until we are finished */

    bzero(ler2, sizeof(*ler2));

    ler2->ler2_mode = LE_MODE;
    ler2->ler2_padr[0] = myea[1];
    ler2->ler2_padr[1] = myea[0];
    ler2->ler2_padr[2] = myea[3];
    ler2->ler2_padr[3] = myea[2];
    ler2->ler2_padr[4] = myea[5];
    ler2->ler2_padr[5] = myea[4];


    ler2->ler2_ladrf0 = 0;
    ler2->ler2_ladrf1 = 0;

    a = LANCE_ADDR(ler2->ler2_rmd);
#ifdef RECV_DEBUG
    ler2->ler2_rlen =  0 | (a >> 16);
#undef LERBUF
#define LERBUF 1
#else
    ler2->ler2_rlen =  LE_RLEN | (a >> 16);
#endif
    ler2->ler2_rdra = a & LE_ADDR_LOW_MASK; 

    a = LANCE_ADDR(ler2->ler2_tmd);
    ler2->ler2_tlen = LE_TLEN | (a >> 16);
    ler2->ler2_tdra = a & LE_ADDR_LOW_MASK;

    ler1->ler1_rap = LE_CSR1;
    a = LANCE_ADDR(ler2);
    ler1->ler1_rdp = a & LE_ADDR_LOW_MASK;
    ler1->ler1_rap = LE_CSR2;
    ler1->ler1_rdp = a >> 16;

    for (i = 0; i < LERBUF; i++) {
	a = LANCE_ADDR(&ler2->ler2_rbuf[i]);
	ler2->ler2_rmd[i].rmd0 = a & LE_ADDR_LOW_MASK;
	ler2->ler2_rmd[i].rmd1_bits = LE_OWN;
	ler2->ler2_rmd[i].rmd1_hadr = a >> 16;
	if (le_debug)
	    printf("le rbuf[%d] = %x%x\n", i, a >>16, a & LE_ADDR_LOW_MASK);
	ler2->ler2_rmd[i].rmd2 = -LEMTU;
	ler2->ler2_rmd[i].rmd3 = 0;
    }
    for (i = 0; i < LETBUF; i++) {
	a = LANCE_ADDR(&ler2->ler2_tbuf[i]);
	ler2->ler2_tmd[i].tmd0 = a & LE_ADDR_LOW_MASK;
	ler2->ler2_tmd[i].tmd1_bits = 0;
	ler2->ler2_tmd[i].tmd1_hadr = a >> 16;
	if (le_debug)
	    printf("le tbuf[%d] = %x%x\n", i, a >>16, a & LE_ADDR_LOW_MASK );
	ler2->ler2_tmd[i].tmd2 = 0;
	ler2->ler2_tmd[i].tmd3 = 0;
    }

    ler1->ler1_rap = LE_CSR3;
    ler1->ler1_rdp = LE_BSWP;

    ler1->ler1_rap = LE_CSR0;
    ler1->ler1_rdp = LE_INIT;
    do {
	if (--timo == 0) {
	    printf("le%d: init timeout, stat = 0x%x\n",
		   le_netif.netif_unit, stat);
	    break;
	}
	stat = ler1->ler1_rdp;
    } while ((stat & LE_IDON) == 0);
    
    ler1->ler1_rdp = LE_IDON;
    le_softc.next_rmd = 0;
    le_softc.next_tmd = 0;
    ler1->ler1_rap = LE_CSR0;
    ler1->ler1_rdp = LE_STRT;
    le_mem_summary();
}

int le_poll(desc, pkt, len)
     struct iodesc *desc;
     void *pkt;
     int len;
{
    struct lereg1 *ler1 = le_softc.sc_r1;
    struct lereg2 *ler2 = le_softc.sc_r2;
    unsigned int a;
    int length;
    struct lermd *rmd;


    printf("next_rmd on poll attempt %d\n", le_softc.next_rmd);
    ler1->ler1_rap = LE_CSR0;
    if ((ler1->ler1_rdp & LE_RINT) == 0)
	return 0;
    ler1->ler1_rdp = LE_RINT;
    rmd = &ler2->ler2_rmd[le_softc.next_rmd];
    if (le_debug) {
	printf("next_rmd %d\n", le_softc.next_rmd);
	printf("rmd->rmd1_bits %x\n", rmd->rmd1_bits);
	printf("rmd->rmd2 %x, rmd->rmd3 %x\n", rmd->rmd2, rmd->rmd3);
	printf("rmd->rbuf msg %d buf %d\n", rmd->rmd3, -rmd->rmd2 );
    }
    if (rmd->rmd1_bits & LE_OWN)
	panic("le_poll: rmd still owned by lance");
    if (ler1->ler1_rdp & LE_SERR)
	le_error("le_poll", ler1);
    if (rmd->rmd1_bits & LE_ERR) {
	printf("le%d_poll: rmd status 0x%x\n", rmd->rmd1_bits);
	length = 0;
	goto cleanup;
    }
    if ((rmd->rmd1_bits & (LE_STP|LE_ENP)) != (LE_STP|LE_ENP))
	panic("le_poll: chained packet\n");

    length = rmd->rmd3;
    printf("le_poll: length %d\n", length);
    if (length >= LEMTU) {
	length = 0;
	panic("csr0 when bad things happen: %x\n", ler1->ler1_rdp);
	goto cleanup;
    }
    if (!length) goto cleanup;
    length -= 4;
    if (length > 0)
	bcopy(&ler2->ler2_rbuf[le_softc.next_rmd], pkt, length);

 cleanup: 
    le_sanity_check("before forced rmd sanity");
    a = LANCE_ADDR(&ler2->ler2_rbuf[le_softc.next_rmd]);
    rmd->rmd0 = a & LE_ADDR_LOW_MASK;
    rmd->rmd1_hadr = a >> 16;
    rmd->rmd2 = -LEMTU;
    le_softc.next_rmd =
	(le_softc.next_rmd == (LERBUF - 1)) ? 0 : (le_softc.next_rmd + 1);
    printf("new next_rmd %d\n", le_softc.next_rmd);
    le_sanity_check("after forced rmd sanity");
    rmd->rmd1_bits = LE_OWN;
    return length;
}

int le_put(desc, pkt, len)
     struct iodesc *desc;
     void *pkt;
     int len;
{
    volatile struct lereg1 *ler1 = le_softc.sc_r1;
    volatile struct lereg2 *ler2 = le_softc.sc_r2;
    volatile struct letmd *tmd;
    int timo = 100000, stat, i;
    unsigned int a;

    if (le_debug)
	printf("le%d: le_put called\n", LE_UNIT);
    printf("wierd place le_next_rmd %d\n", le_softc.next_rmd);
    le_sanity_check("before transmit");
    ler1->ler1_rap = LE_CSR0;
    if (ler1->ler1_rdp & LE_SERR)
	le_error("le_put(way before xmit)", ler1);
    tmd = &ler2->ler2_tmd[le_softc.next_tmd];
    while(tmd->tmd1_bits & LE_OWN) {
	printf("le%d: output buffer busy\n");
    }
    bcopy(pkt, ler2->ler2_tbuf[le_softc.next_tmd], len);
    if (len < 64) 
	tmd->tmd2 = -64;
    else 
	tmd->tmd2 = -len;
    tmd->tmd3 = 0;
    if (ler1->ler1_rdp & LE_SERR)
	le_error("le_put(before xmit)", ler1);
    tmd->tmd1_bits = LE_STP | LE_ENP | LE_OWN;
    a = LANCE_ADDR(&ler2->ler2_tbuf[le_softc.next_tmd]);
    tmd->tmd0 = a & LE_ADDR_LOW_MASK;
    tmd->tmd1_hadr = a >> 16;
    ler1->ler1_rdp = LE_TDMD;
    if (ler1->ler1_rdp & LE_SERR)
	le_error("le_put(after xmit)", ler1);
    do {
	if (--timo == 0) {
	    printf("le%d: transmit timeout, stat = 0x%x\n",
		   le_netif.netif_unit, stat);
	    if (ler1->ler1_rdp & LE_SERR)
		le_error("le_put(timeout)", ler1);
	    break;
	}
	stat = ler1->ler1_rdp;
    } while ((stat & LE_TINT) == 0);
    ler1->ler1_rdp = LE_TINT;
    if (ler1->ler1_rdp & LE_SERR) {
	printf("le_put: xmit error, buf %d\n", le_softc.next_tmd);
	le_error("le_put(xmit error)", ler1);
    }
    le_sanity_check("after transmit");
    le_softc.next_tmd = 0;
    le_sanity_check("after next tmd");
/*	(le_softc.next_tmd == (LETBUF - 1)) ? 0 : le_softc.next_tmd + 1;*/
    if (tmd->tmd1_bits & LE_DEF) le_stats.deferred++;
    if (tmd->tmd1_bits & LE_ONE) le_stats.collisions++;
    if (tmd->tmd1_bits & LE_MORE) le_stats.collisions+=2;
    le_sanity_check("bits check");
    if (tmd->tmd1_bits & LE_ERR) {
	printf("le%d: transmit error, error = 0x%x\n", LE_UNIT,
	       tmd->tmd3);
	return -1;
    }
    le_sanity_check("le_debug check");
    if (le_debug) {
	printf("le%d: le_put() successful: sent %d\n", LE_UNIT, len);
	printf("le%d: le_put(): tmd1_bits: %x tmd3: %x\n", LE_UNIT,
	       (unsigned int) tmd->tmd1_bits,
	       (unsigned int) tmd->tmd3);
    }
    le_sanity_check("after le_put return len");
    return len;
}
int le_get(desc, pkt, len, timeout)
     struct iodesc *desc;
     void *pkt;
     int len;
     time_t timeout;
{
    time_t t;
    int cc;

    t = getsecs();
    cc = 0;
    while (((getsecs() - t) < timeout) && !cc) {
	cc = le_poll(desc, pkt, len);
    }
    return cc;
}

void le_init(desc, machdep_hint)
    struct iodesc *desc;
    void *machdep_hint;
{
    caddr_t addr;

    if (le_debug)
	printf("le%d: le_init called\n", LE_UNIT);
    bzero(&le_softc, sizeof(le_softc));
    addr = obio_alloc((caddr_t) OBIO_AMD_ETHER, OBIO_AMD_ETHER_SIZE,
		      OBIO_WRITE);
    if (addr == NULL)
	panic("le%d: out of obio memory???", le_netif.netif_unit);
    le_softc.sc_r1 = (struct lereg1 *) addr;
    addr = dvma_malloc(sizeof(struct lereg2));
    if (addr == NULL)
	panic("le%d: no dvma space???", le_netif.netif_unit);
    le_softc.sc_r2 = (struct lereg2 *) addr;
    le_reset(desc->myea);
}

void le_end()
{
    struct lereg1 *ler1 = le_softc.sc_r1;

    if (le_debug)
	printf("le%d: le_end called\n", LE_UNIT);
    ler1->ler1_rap = LE_CSR0;
    ler1->ler1_rdp = LE_STOP;

    obio_free(le_softc.sc_r1);
    dvma_free(le_softc.sc_r2);
}
