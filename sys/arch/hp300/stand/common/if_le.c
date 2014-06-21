/*	$NetBSD: if_le.c,v 1.13 2014/06/21 02:02:40 tsutsui Exp $	*/

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
 */

#include <sys/param.h>
#include <sys/types.h>

#include <net/if_ether.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>

#include <lib/libkern/libkern.h>

#include <hp300/stand/common/device.h>
#include <hp300/stand/common/if_lereg.h>
#include <hp300/stand/common/samachdep.h>

#ifndef NLE
#define NLE 1
#endif

struct le_softc {
	struct	lereg0 *sc_r0;	/* DIO registers */
	struct	lereg1 *sc_r1;	/* LANCE registers */
	void	*sc_mem;
	struct	init_block *sc_init;
	struct	mds *sc_rd, *sc_td;
	u_char	*sc_rbuf, *sc_tbuf;
	int	sc_next_rd, sc_next_td;
	u_char	sc_addr[ETHER_ADDR_LEN];
};

struct le_sel {
	int	le_id;
	int	le_regs;
	int	le_mem;
	int	le_nvram;
	int	le_heat;
	int	le_bonus;
};

int le_probe(struct netif *, void *);
int le_match(struct netif *, void *);
void le_init(struct iodesc *, void *);
int le_get(struct iodesc *, void *, size_t, saseconds_t);
int le_put(struct iodesc *, void *, size_t);
void le_end(struct netif *);

static inline void lewrcsr(struct le_softc *, uint16_t, uint16_t);
static inline uint16_t lerdcsr(struct le_softc *, uint16_t);

static void leinit(void);
static void le_error(int, char *, uint16_t);
static void lememinit(struct le_softc *);
static void le_reset(int, u_char *);
static int le_poll(struct iodesc *, void *, int);

#ifdef LE_DEBUG
int le_debug = 0;
#endif

struct le_sel le0conf[] = {
/* offsets for:	   ID   REGS     MEM   NVRAM	le_heat	le_bonus*/
{		    0,	0x4000, 0x8000, 0xC008,	1,	10   }
};
#define NLE0CONF (sizeof(le0conf) / sizeof(le0conf[0]))

extern struct netif_stats	le_stats[];

struct netif_dif le_ifs[] = {
/*	dif_unit	dif_nsel	dif_stats	dif_private	*/
{	0,		NLE0CONF,	&le_stats[0],	le0conf,	},
};
#define NLE_IFS (sizeof(le_ifs) / sizeof(le_ifs[0]))

struct netif_stats le_stats[NLE_IFS];

struct netif_driver le_driver = {
	"le",			/* netif_bname */
	le_match,		/* netif_match */
	le_probe,		/* netif_probe */
	le_init,		/* netif_init */
	le_get,			/* netif_get */
	le_put,			/* netif_put */
	le_end,			/* netif_end */
	le_ifs,			/* netif_ifs */
	NLE_IFS			/* netif_nifs */
};

struct le_softc le_softc[NLE];

static inline void
lewrcsr(struct le_softc *sc, uint16_t port, uint16_t val)
{
	struct lereg0 *ler0 = sc->sc_r0;
	struct lereg1 *ler1 = sc->sc_r1;

	do {
		ler1->ler1_rap = port;
	} while ((ler0->ler0_status & LE_ACK) == 0);
	do {
		ler1->ler1_rdp = val;
	} while ((ler0->ler0_status & LE_ACK) == 0);
}

static inline uint16_t
lerdcsr(struct le_softc *sc, uint16_t port)
{
	struct lereg0 *ler0 = sc->sc_r0;
	struct lereg1 *ler1 = sc->sc_r1;
	uint16_t val;

	do {
		ler1->ler1_rap = port;
	} while ((ler0->ler0_status & LE_ACK) == 0);
	do {
		val = ler1->ler1_rdp;
	} while ((ler0->ler0_status & LE_ACK) == 0);
	return val;
}

static void
leinit(void)
{
	struct hp_hw *hw;
	struct le_softc *sc;
	struct le_sel *sels;
	int i, n;
	char *cp;

	i = 0;

	for (hw = sc_table; i < NLE && hw < &sc_table[MAXCTLRS]; hw++) {
#ifdef LE_DEBUG
		if (le_debug)
			printf("found type %x\n", hw->hw_type);
#endif

#if 0
		if (!HW_ISDEV(hw, D_LAN))
			continue;
#endif

		sels = (struct le_sel *)le_ifs[i].dif_private;

		sc = &le_softc[i];
		sc->sc_r0 = (struct lereg0 *)(sels->le_id + (int)hw->hw_kva);

		if (sc->sc_r0->ler0_id != LEID)
			continue;

		sc->sc_r1 = (struct lereg1 *)(sels->le_regs + (int)hw->hw_kva);
		sc->sc_mem = (struct lereg2 *)(sels->le_mem + (int)hw->hw_kva);

#ifdef LE_DEBUG
		if (le_debug)
			printf("le%d: DIO=%x regs=%x mem=%x\n",
				i, sc->sc_r0, sc->sc_r1, sc->sc_mem);
#endif

		/*
		 * Read the ethernet address off the board, one nibble at a time.
		 */
		cp = (char *)(sels->le_nvram + (int)hw->hw_kva);
		for (n = 0; n < sizeof(sc->sc_addr); n++) {
		    sc->sc_addr[n] = (*++cp & 0xF) << 4;
		    cp++;
		    sc->sc_addr[n] |= *++cp & 0xF;
		    cp++;
		}
#ifdef LE_DEBUG
		if (le_debug)
			printf("le%d at sc%d physical address %s\n",
				i, hw->hw_sc, ether_sprintf(sc->sc_addr));
#endif
		hw->hw_pa = (void *) i;	/* XXX for autoconfig */
		i++;
	}
}

int
le_match(struct netif *nif, void *machdep_hint)
{
	struct le_sel *sels;
	char *name = machdep_hint;
	int rv = 0;

	if (nif->nif_sel < le_ifs[nif->nif_unit].dif_nsel) {
		sels = (struct le_sel *)le_ifs[nif->nif_unit].dif_private;
		rv = sels[nif->nif_sel].le_heat;
		if (name && !strncmp(le_driver.netif_bname, name, 2))
			rv += sels[nif->nif_sel].le_bonus;
	}
#ifdef LE_DEBUG
	if (le_debug)
		printf("le%d: sel %d --> %d\n", nif->nif_unit, nif->nif_sel,
		    rv);
#endif
	return rv;
}

int
le_probe(struct netif *nif, void *machdep_hint)
{
#if 0
	char *cp;
	int i;
#endif

	/* the set unit is the current unit */
#ifdef LE_DEBUG
	if (le_debug)
		printf("le%d.%d: le_probe called\n", nif->nif_unit, nif->nif_sel);
#endif
	/* XXX reset controller */
	return 0;
}

#ifdef MEM_SUMMARY
void
le_mem_summary(int unit)
{
	struct lereg1 *ler1 = le_softc.sc_r1;
	struct lereg2 *ler2 = le_softc.sc_r2;
	int i;

	printf("le%d: ler1 = %x\n", unit, ler1);
	printf("le%d: ler2 = %x\n", unit, ler2);

#if 0
	ler1->ler1_rap = LE_CSR0;
	ler1->ler1_rdp = LE_STOP;
	printf("le%d: csr0 = %x\n", unit, ler1->ler1_rdp);
	ler1->ler1_rap = LE_CSR1;
	printf("le%d: csr1 = %x\n", unit, ler1->ler1_rdp);
	ler1->ler1_rap = LE_CSR2;
	printf("le%d: csr2 = %x\n", unit, ler1->ler1_rdp);
	ler1->ler1_rap = LE_CSR3;
	printf("le%d: csr3 = %x\n", unit, ler1->ler1_rdp);
#endif
	printf("le%d: ladrf[0] = %x\n", unit, ler2->ler2_ladrf[0]);
	printf("le%d: ladrf[1] = %x\n", unit, ler2->ler2_ladrf[1]);
	printf("le%d: ler2_rdra = %x\n", unit, ler2->ler2_rdra);
	printf("le%d: ler2_rlen = %x\n", unit, ler2->ler2_rlen);
	printf("le%d: ler2_tdra = %x\n", unit, ler2->ler2_tdra);
	printf("le%d: ler2_tlen = %x\n", unit, ler2->ler2_tlen);

	for (i = 0; i < LERBUF; i++) {
		printf("le%d: ler2_rmd[%d].rmd0 (ladr) = %x\n", unit, i,
			ler2->ler2_rmd[i].rmd0);
		printf("le%d: ler2_rmd[%d].rmd1 = %x\n", unit, i,
			ler2->ler2_rmd[i].rmd1);
		printf("le%d: ler2_rmd[%d].rmd2 (-bcnt) = %x\n", unit, i,
			ler2->ler2_rmd[i].rmd2);
		printf("le%d: ler2_rmd[%d].rmd3 (mcnt) = %x\n", unit, i,
			ler2->ler2_rmd[i].rmd3);
		printf("le%d: ler2_rbuf[%d] addr = %x\n", unit, i,
			&ler2->ler2_rbuf[i]);
	}
	for (i = 0; i < LETBUF; i++) {
		printf("le%d: ler2_tmd[%d].tmd0 = %x\n", unit, i,
			ler2->ler2_tmd[i].tmd0);
		printf("le%d: ler2_tmd[%d].tmd1 = %x\n", unit, i,
			ler2->ler2_tmd[i].tmd1);
		printf("le%d: ler2_tmd[%d].tmd2 (bcnt) = %x\n", unit, i,
			ler2->ler2_tmd[i].tmd2);
		printf("le%d: ler2_tmd[%d].tmd3 = %x\n", unit, i,
			ler2->ler2_tmd[i].tmd3);
		printf("le%d: ler2_tbuf[%d] addr = %x\n", unit, i,
			&ler2->ler2_tbuf[i]);
	}
}
#else
#define le_mem_summary(u)
#endif

void
le_error(int unit, char *str, uint16_t stat)
{

	if (stat & LE_BABL)
		panic("le%d: been babbling, found by '%s'", unit, str);
	if (stat & LE_CERR)
		le_stats[unit].collision_error++;
	if (stat & LE_MISS)
		le_stats[unit].missed++;
	if (stat & LE_MERR) {
		printf("le%d: memory error in '%s'\n", unit, str);
		le_mem_summary(unit);
		panic("bye");
	}
}

#define	LANCE_ADDR(sc, a) \
	((u_long)(a) - (u_long)sc->sc_mem)

/* LANCE initialization block set up. */
void
lememinit(struct le_softc *sc)
{
	int i;
	u_char *mem;
	u_long a;

	/*
	 * At this point we assume that the memory allocated to the Lance is
	 * quadword aligned.  If it isn't then the initialisation is going
	 * fail later on.
	 */
	mem = sc->sc_mem;

	sc->sc_init = (void *)mem;
	sc->sc_init->mode = LE_NORMAL;
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		sc->sc_init->padr[i] = sc->sc_addr[i^1];
	sc->sc_init->ladrf[0] = sc->sc_init->ladrf[1] = 0;
	mem += sizeof(struct init_block);

	sc->sc_rd = (void *)mem;
	a = LANCE_ADDR(sc, mem);
	sc->sc_init->rdra = a;
	sc->sc_init->rlen = ((a >> 16) & 0xff) | (RLEN << 13);
	mem += NRBUF * sizeof(struct mds);

	sc->sc_td = (void *)mem;
	a = LANCE_ADDR(sc, mem);
	sc->sc_init->tdra = a;
	sc->sc_init->tlen = ((a >> 16) & 0xff) | (TLEN << 13);
	mem += NTBUF * sizeof(struct mds);

	/*
	 * Set up receive ring descriptors.
	 */
	sc->sc_rbuf = mem;
	for (i = 0; i < NRBUF; i++) {
		a = LANCE_ADDR(sc, mem);
		sc->sc_rd[i].addr = a;
		sc->sc_rd[i].flags = ((a >> 16) & 0xff) | LE_OWN;
		sc->sc_rd[i].bcnt = -BUFSIZE;
		sc->sc_rd[i].mcnt = 0;
		mem += BUFSIZE;
	}

	/*
	 * Set up transmit ring descriptors.
	 */
	sc->sc_tbuf = mem;
	for (i = 0; i < NTBUF; i++) {
		a = LANCE_ADDR(sc, mem);
		sc->sc_td[i].addr = a;
		sc->sc_td[i].flags = ((a >> 16) & 0xff);
		sc->sc_td[i].bcnt = 0xf000;
		sc->sc_td[i].mcnt = 0;
		mem += BUFSIZE;
	}
}

void
le_reset(int unit, u_char *myea)
{
	struct le_softc *sc = &le_softc[unit];
	u_long a;
	int timo = 100000;

#ifdef LE_DEBUG
	if (le_debug) {
		printf("le%d: le_reset called\n", unit);
		printf("     r0=%x, r1=%x, mem=%x, addr=%x:%x:%x:%x:%x:%x\n",
		       sc->sc_r0, sc->sc_r1, sc->sc_mem,
		       sc->sc_addr[0], sc->sc_addr[1], sc->sc_addr[2],
		       sc->sc_addr[3], sc->sc_addr[4], sc->sc_addr[5]);
	}
#endif
	lewrcsr(sc, 0, LE_STOP);
	for (timo = 1000; timo; timo--);

	sc->sc_next_rd = sc->sc_next_td = 0;

	/* Set up LANCE init block. */
	lememinit(sc);

	if (myea)
		memcpy(myea, sc->sc_addr, ETHER_ADDR_LEN);

	/* Turn on byte swapping. */
	lewrcsr(sc, 3, LE_BSWP);

	/* Give LANCE the physical address of its init block. */
	a = LANCE_ADDR(sc, sc->sc_init);
	lewrcsr(sc, 1, a);
	lewrcsr(sc, 2, (a >> 16) & 0xff);

#ifdef LE_DEBUG
	if (le_debug)
		printf("le%d: before init\n", unit);
#endif

	/* Try to initialize the LANCE. */
	lewrcsr(sc, 0, LE_INIT);

	/* Wait for initialization to finish. */
	for (timo = 100000; timo; timo--)
		if (lerdcsr(sc, 0) & LE_IDON)
			break;

	if (lerdcsr(sc, 0) & LE_IDON) {
		/* Start the LANCE. */
		lewrcsr(sc, 0, LE_INEA | LE_STRT | LE_IDON);
	} else
		printf("le%d: card failed to initialize\n", unit);

#ifdef LE_DEBUG
	if (le_debug)
		printf("le%d: after init\n", unit);
#endif

	le_mem_summary(unit);
}

int
le_poll(struct iodesc *desc, void *pkt, int len)
{
	int unit = /*nif->nif_unit*/0;
	struct le_softc *sc = &le_softc[unit];
	int length;
	volatile struct mds *cdm;
	int stat;

#ifdef LE_DEBUG
	if (/*le_debug*/0)
		printf("le%d: le_poll called. next_rd=%d\n", unit, sc->sc_next_rd);
#endif
	stat = lerdcsr(sc, 0);
	lewrcsr(sc, 0, stat & (LE_BABL | LE_MISS | LE_MERR | LE_RINT));
	cdm = &sc->sc_rd[sc->sc_next_rd];
	if (cdm->flags & LE_OWN)
		return 0;
#ifdef LE_DEBUG
	if (le_debug) {
		printf("next_rd %d\n", sc->sc_next_rd);
		printf("cdm->flags %x\n", cdm->flags);
		printf("cdm->bcnt %x, cdm->mcnt %x\n", cdm->bcnt, cdm->mcnt);
		printf("cdm->rbuf msg %d buf %d\n", cdm->mcnt, -cdm->bcnt );
	}
#endif
	if (stat & (LE_BABL | LE_CERR | LE_MISS | LE_MERR))
		le_error(unit, "le_poll", stat);
	if (cdm->flags & (LE_FRAM | LE_OFLO | LE_CRC | LE_RBUFF)) {
		printf("le%d_poll: rmd status 0x%x\n", unit, cdm->flags);
		length = 0;
		goto cleanup;
	}
	if ((cdm->flags & (LE_STP|LE_ENP)) != (LE_STP|LE_ENP))
		panic("le_poll: chained packet");

	length = cdm->mcnt;
#ifdef LE_DEBUG
	if (le_debug)
		printf("le_poll: length %d\n", length);
#endif
	if (length >= BUFSIZE) {
		length = 0;
		panic("csr0 when bad things happen: %x", stat);
		goto cleanup;
	}
	if (!length)
		goto cleanup;
	length -= 4;

	if (length > 0) {
		/*
		 * If the length of the packet is greater than the size of the
		 * buffer, we have to truncate it, to avoid Bad Things.
		 * XXX Is this the right thing to do?
		 */
		if (length > len)
			length = len;

		memcpy(pkt, sc->sc_rbuf + (BUFSIZE * sc->sc_next_rd), length);
	}

cleanup:
	cdm->mcnt = 0;
	cdm->flags |= LE_OWN;
	if (++sc->sc_next_rd >= NRBUF)
		sc->sc_next_rd = 0;
#ifdef LE_DEBUG
	if (le_debug)
		printf("new next_rd %d\n", sc->sc_next_rd);
#endif

	return length;
}

int
le_put(struct iodesc *desc, void *pkt, size_t len)
{
	int unit = /*nif->nif_unit*/0;
	struct le_softc *sc = &le_softc[unit];
	volatile struct mds *cdm;
	int timo, stat;
#if 0
	int i;
#endif

 le_put_loop:
	timo = 100000;

#ifdef LE_DEBUG
	if (le_debug)
		printf("le%d: le_put called. next_td=%d\n", unit, sc->sc_next_td);
#endif
	stat = lerdcsr(sc, 0);
	lewrcsr(sc, 0, stat & (LE_BABL | LE_MISS | LE_MERR | LE_TINT));
	if (stat & (LE_BABL | LE_CERR | LE_MISS | LE_MERR))
		le_error(unit, "le_put(way before xmit)", stat);
	cdm = &sc->sc_td[sc->sc_next_td];
#if 0
	i = 0;
	while (cdm->flags & LE_OWN) {
		if ((i % 100) == 0)
			printf("le%d: output buffer busy - flags=%x\n",
				unit, cdm->flags);
		if (i++ > 500) break;
	}
	if (cdm->flags & LE_OWN)
		getchar();
#else
	while (cdm->flags & LE_OWN);
#endif
	memcpy(sc->sc_tbuf + (BUFSIZE * sc->sc_next_td), pkt, len);
	if (len < ETHER_MIN_LEN)
		cdm->bcnt = -ETHER_MIN_LEN;
	else
		cdm->bcnt = -len;
	cdm->mcnt = 0;
	cdm->flags |= LE_OWN | LE_STP | LE_ENP;
	stat = lerdcsr(sc, 0);
	if (stat & (LE_BABL | LE_CERR | LE_MISS | LE_MERR))
		le_error(unit, "le_put(before xmit)", stat);
	lewrcsr(sc, 0, LE_TDMD);
	stat = lerdcsr(sc, 0);
	if (stat & (LE_BABL | LE_CERR | LE_MISS | LE_MERR))
		le_error(unit, "le_put(after xmit)", stat);
	do {
		if (--timo == 0) {
			printf("le%d: transmit timeout, stat = 0x%x\n",
				unit, stat);
			if (stat & LE_SERR)
				le_error(unit, "le_put(timeout)", stat);
			if (stat & LE_INIT) {
				printf("le%d: reset and retry packet\n", unit);
				lewrcsr(sc, 0, LE_TINT);	/* sanity */
				leinit();
				goto le_put_loop;
			}
			break;
		}
		stat = lerdcsr(sc, 0);
	} while ((stat & LE_TINT) == 0);
	lewrcsr(sc, 0, LE_TINT);
	if (stat & (LE_BABL |/* LE_CERR |*/ LE_MISS | LE_MERR)) {
		printf("le_put: xmit error, buf %d\n", sc->sc_next_td);
		le_error(unit, "le_put(xmit error)", stat);
	}
	if (++sc->sc_next_td >= NTBUF)
		sc->sc_next_td = 0;
	if (cdm->flags & LE_DEF)
		le_stats[unit].deferred++;
	if (cdm->flags & LE_ONE)
		le_stats[unit].collisions++;
	if (cdm->flags & LE_MORE)
		le_stats[unit].collisions += 2;
	if (cdm->flags & LE_ERR) {
		if (cdm->mcnt & LE_UFLO)
			printf("le%d: transmit underflow\n", unit);
		if (cdm->mcnt & LE_LCOL)
			le_stats[unit].collisions++;
		if (cdm->mcnt & LE_LCAR)
			printf("le%d: lost carrier\n", unit);
		if (cdm->mcnt & LE_RTRY)
			le_stats[unit].collisions += 16;
		return -1;
	}
#ifdef LE_DEBUG
	if (le_debug) {
		printf("le%d: le_put() successful: sent %d\n", unit, len);
		printf("le%d: le_put(): flags: %x mcnt: %x\n", unit,
			(unsigned int) cdm->flags,
			(unsigned int) cdm->mcnt);
	}
#endif
	return len;
}


int
le_get(struct iodesc *desc, void *pkt, size_t len, saseconds_t timeout)
{
	satime_t t;
	int cc;

	t = getsecs();
	do {
		cc = le_poll(desc, pkt, len);
	} while (cc == 0 && (getsecs() - t) < timeout);
	return cc;
}

void
le_init(struct iodesc *desc, void *machdep_hint)
{
	struct netif *nif = desc->io_netif;
	int unit = nif->nif_unit;

	/* Get machine's common ethernet interface. This is done in leinit() */
	/* machdep_common_ether(myea); */
	leinit();

#ifdef LE_DEBUG
	if (le_debug)
		printf("le%d: le_init called\n", unit);
#endif
	unit = 0;
	le_reset(unit, desc->myea);
}

void
le_end(struct netif *nif)
{
	int unit = nif->nif_unit;

#ifdef LE_DEBUG
	if (le_debug)
		printf("le%d: le_end called\n", unit);
#endif

	lewrcsr(&le_softc[unit], 0, LE_STOP);
}
