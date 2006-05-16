/*	$NetBSD: gtmpsc.c,v 1.18 2006/05/16 16:49:41 he Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * mpsc.c - MPSC serial driver, supports UART mode only
 *
 *
 * creation	Mon Apr  9 19:40:15 PDT 2001	cliff
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gtmpsc.c,v 1.18 2006/05/16 16:49:41 he Exp $");

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kauth.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/callout.h>
#include <sys/fcntl.h>
#ifdef KGDB
#include <sys/kernel.h>
#include <sys/kgdb.h>
#endif

#include <uvm/uvm_extern.h>

#include <powerpc/atomic.h>
#include <dev/cons.h>
#include <machine/bus.h>
#include <machine/cpu.h>		/* for DELAY */
#include <machine/stdarg.h>
#include "gtmpsc.h"


#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtintrreg.h>
#include <dev/marvell/gtmpscreg.h>
#include <dev/marvell/gtsdmareg.h>
#include <dev/marvell/gtmpscvar.h>
#include <dev/marvell/gtbrgreg.h>

/*
 * XXX these delays were derived empiracaly
 */
#define GTMPSC_POLL_DELAY	1	/* 1 usec */
/*
 * Wait 2 characters time for RESET_DELAY
 */
#define GTMPSC_RESET_DELAY	(2*8*1000000 / GT_MPSC_DEFAULT_BAUD_RATE)

#define BURSTLEN 128

/*
 * stat values for gtmpsc_common_pollc
 */
#define GTMPSC_STAT_NONE	0
#define GTMPSC_STAT_BREAK	1


#define PRINTF(x)	gtmpsc_mem_printf x

#if defined(DEBUG)
unsigned int gtmpsc_debug = 0;
# define STATIC
# define DPRINTF(x)	do { if (gtmpsc_debug) gtmpsc_mem_printf x ; } while (0)
#else
# define STATIC static
# define DPRINTF(x)
#endif

#define GTMPSCUNIT_MASK    0x7ffff
#define GTMPSCDIALOUT_MASK 0x80000

#define GTMPSCUNIT(x)      (minor(x) & GTMPSCUNIT_MASK)
#define GTMPSCDIALOUT(x)   (minor(x) & GTMPSCDIALOUT_MASK)

STATIC void gtmpscinit(struct gtmpsc_softc *);
STATIC int  gtmpscmatch(struct device *, struct cfdata *, void *);
STATIC void gtmpscattach(struct device *, struct device *, void *);
STATIC int  compute_cdv(unsigned int);
STATIC void gtmpsc_loadchannelregs(struct gtmpsc_softc *);
STATIC void gtmpscshutdown(struct gtmpsc_softc *);
STATIC void gtmpscstart(struct tty *);
STATIC int  gtmpscparam(struct tty *, struct termios *);
STATIC int  gtmpsc_probe(void);
STATIC int  gtmpsc_intr(void *);
STATIC void gtmpsc_softintr(void *);

STATIC void gtmpsc_common_putn(struct gtmpsc_softc *);
STATIC void gtmpsc_common_putc(unsigned int, unsigned char);
STATIC int  gtmpsc_common_getc(unsigned int);
STATIC int  gtmpsc_common_pollc(unsigned int, char *, int *);
STATIC void gtmpsc_poll(void *);
#ifdef KGDB
STATIC void gtmpsc_kgdb_poll(void *);
#endif
STATIC void gtmpsc_mem_printf(const char *, ...);

STATIC void gtmpsc_txdesc_init(gtmpsc_poll_sdma_t *, gtmpsc_poll_sdma_t *);
STATIC void gtmpsc_rxdesc_init(gtmpsc_poll_sdma_t *, gtmpsc_poll_sdma_t *);
STATIC unsigned int gtmpsc_get_causes(void);
STATIC void gtmpsc_hackinit(struct gtmpsc_softc *, bus_space_tag_t,
	bus_space_handle_t, int);
STATIC void gtmpscinit_stop(struct gtmpsc_softc *, int);
STATIC void gtmpscinit_start(struct gtmpsc_softc *, int);
#if 0
void gtmpsc_printf(const char *fmt, ...);
#endif
void gtmpsc_puts(char *);

void gtmpsccnprobe(struct consdev *);
void gtmpsccninit(struct consdev *);
int  gtmpsccngetc(dev_t);
void gtmpsccnputc(dev_t, int);
void gtmpsccnpollc(dev_t, int);
void gtmpsccnhalt(dev_t);

STATIC void gtmpsc_txflush(gtmpsc_softc_t *);
STATIC void gtmpsc_iflush(gtmpsc_softc_t *);
STATIC void gtmpsc_shutdownhook(void *);

dev_type_open(gtmpscopen);
dev_type_close(gtmpscclose);
dev_type_read(gtmpscread);
dev_type_write(gtmpscwrite);
dev_type_ioctl(gtmpscioctl);
dev_type_stop(gtmpscstop);
dev_type_tty(gtmpsctty);
dev_type_poll(gtmpscpoll);

const struct cdevsw gtmpsc_cdevsw = {
	gtmpscopen, gtmpscclose, gtmpscread, gtmpscwrite, gtmpscioctl,
	gtmpscstop, gtmpsctty, gtmpscpoll, nommap, ttykqfilter, D_TTY
};

CFATTACH_DECL(gtmpsc, sizeof(struct gtmpsc_softc),
    gtmpscmatch, gtmpscattach, NULL, NULL);

extern struct cfdriver gtmpsc_cd;

static struct consdev gtmpsc_consdev = {
	0,
	gtmpsccninit,
	gtmpsccngetc,
	gtmpsccnputc,
	gtmpsccnpollc,
	NULL,		/* cn_bell */
	gtmpsccnhalt,
	NULL,		/* cn_flush */
	NODEV,
	CN_NORMAL
};

STATIC void *gtmpsc_sdma_ih = NULL;

gtmpsc_softc_t *gtmpsc_scp[GTMPSC_NCHAN] = { 0 };

STATIC int gt_reva_gtmpsc_bug;
unsigned int sdma_imask;        /* soft copy of SDMA IMASK reg */

#ifdef KGDB
#include <sys/kgdb.h>

static int gtmpsc_kgdb_addr;
static int gtmpsc_kgdb_attached;

int kgdb_break_immediate /* = 0 */ ;

STATIC int      gtmpsc_kgdb_getc(void *);
STATIC void     gtmpsc_kgdb_putc(void *, int);
#endif /* KGDB */

/*
 * hacks for console initialization
 * which happens prior to autoconfig "attach"
 *
 * XXX Assumes PAGE_SIZE is a constant!
 */
STATIC unsigned int gtmpsccninit_done = 0;
STATIC gtmpsc_softc_t gtmpsc_fake_softc;
STATIC unsigned char gtmpsc_earlybuf[PAGE_SIZE]
    __attribute__ ((aligned(PAGE_SIZE)));
STATIC unsigned char gtmpsc_fake_dmapage[PAGE_SIZE]
    __attribute__ ((aligned(PAGE_SIZE)));


#define GTMPSC_PRINT_BUF_SIZE	4096
STATIC unsigned char gtmpsc_print_buf[GTMPSC_PRINT_BUF_SIZE] = { 0 };

unsigned int gtmpsc_poll_putc_cnt = 0;
unsigned int gtmpsc_poll_putn_cnt = 0;
unsigned int gtmpsc_poll_getc_cnt = 0;
unsigned int gtmpsc_poll_pollc_cnt = 0;
unsigned int gtmpsc_poll_putc_miss = 0;
unsigned int gtmpsc_poll_putn_miss = 0;
unsigned int gtmpsc_poll_getc_miss = 0;
unsigned int gtmpsc_poll_pollc_miss = 0;

#ifndef SDMA_COHERENT
/*
 * inlines to flush, invalidate cache
 * required if DMA cache coherency is broken
 * note that pointer `p' args are assumed to be cache aligned
 * and the size is assumed to be one CACHELINESIZE block
 */

#define GTMPSC_CACHE_FLUSH(p)		gtmpsc_cache_flush(p)
#define GTMPSC_CACHE_INVALIDATE(p)	gtmpsc_cache_invalidate(p)

static volatile inline void
gtmpsc_cache_flush(void *p)
{
	__asm volatile ("eieio; dcbf 0,%0; lwz %0,0(%0); sync;"
					: "+r"(p):);
}

static volatile inline void
gtmpsc_cache_invalidate(void *p)
{
	__asm volatile ("eieio; dcbi 0,%0; sync;" :: "r"(p));
}
#else

#define GTMPSC_CACHE_FLUSH(p)
#define GTMPSC_CACHE_INVALIDATE(p)

#endif	/* SDMA_COHERENT */

#define GT_READ(sc,o) \
	bus_space_read_4((sc)->gtmpsc_memt, (sc)->gtmpsc_memh, (o))
#define GT_WRITE(sc,o,v) \
	bus_space_write_4((sc)->gtmpsc_memt, (sc)->gtmpsc_memh, (o), (v))


#define SDMA_IMASK_ENABLE(sc, bit)  do { \
	unsigned int    __r; \
	GT_WRITE(sc, SDMA_ICAUSE, ~(bit)); \
	if (gt_reva_gtmpsc_bug) \
		__r = sdma_imask; \
	else \
		__r = GT_READ(sc, SDMA_IMASK); \
	__r |= (bit); \
	sdma_imask = __r; \
	GT_WRITE(sc, SDMA_IMASK, __r); \
} while (/*CONSTCOND*/ 0)

#define SDMA_IMASK_DISABLE(sc, bit)  do { \
	unsigned int    __r; \
	if (gt_reva_gtmpsc_bug) \
		__r = sdma_imask; \
	else \
		__r = GT_READ(sc, SDMA_IMASK); \
	__r &= ~(bit); \
	sdma_imask = __r; \
	GT_WRITE(sc, SDMA_IMASK, __r); \
} while (/*CONSTCOND*/ 0)

static volatile inline unsigned int
desc_read(unsigned int *ip)
{
	unsigned int rv;

	__asm volatile ("lwzx %0,0,%1; eieio;"
		: "=r"(rv) : "r"(ip));
	return rv;
}

static volatile inline void
desc_write(unsigned int *ip, unsigned int val)
{
	__asm volatile ("stwx %0,0,%1; eieio;"
		:: "r"(val), "r"(ip));
}


/*
 * gtmpsc_txdesc_init - set up TX descriptor ring
 */
STATIC void
gtmpsc_txdesc_init(gtmpsc_poll_sdma_t *vmps, gtmpsc_poll_sdma_t *pmps)
{
	int n;
	sdma_desc_t *dp;
	gtmpsc_polltx_t *vtxp;
	gtmpsc_polltx_t *ptxp;
	gtmpsc_polltx_t *next_ptxp;
	gtmpsc_polltx_t *first_ptxp;

	first_ptxp = ptxp = &pmps->tx[0];
	vtxp = &vmps->tx[0];
	next_ptxp = ptxp + 1;
	for (n = (GTMPSC_NTXDESC - 1); n--; ) {
		dp = &vtxp->txdesc;
		desc_write(&dp->sdma_csr, 0);
		desc_write(&dp->sdma_cnt, 0);
		desc_write(&dp->sdma_bufp, (u_int32_t)&ptxp->txbuf);
		desc_write(&dp->sdma_next, (u_int32_t)&next_ptxp->txdesc);
		GTMPSC_CACHE_FLUSH(dp);
		vtxp++;
		ptxp++;
		next_ptxp++;
	}
	dp = &vtxp->txdesc;
	desc_write(&dp->sdma_csr, 0);
	desc_write(&dp->sdma_cnt, 0);
	desc_write(&dp->sdma_bufp, (u_int32_t)&ptxp->txbuf);
	desc_write(&dp->sdma_next, (u_int32_t)&first_ptxp->txdesc);
	GTMPSC_CACHE_FLUSH(dp);
}

/*
 * gtmpsc_rxdesc_init - set up RX descriptor ring
 */
STATIC void
gtmpsc_rxdesc_init(gtmpsc_poll_sdma_t *vmps, gtmpsc_poll_sdma_t *pmps)
{
	int n;
	sdma_desc_t *dp;
	gtmpsc_pollrx_t *vrxp;
	gtmpsc_pollrx_t *prxp;
	gtmpsc_pollrx_t *next_prxp;
	gtmpsc_pollrx_t *first_prxp;
	unsigned int csr;

	csr = SDMA_CSR_RX_L|SDMA_CSR_RX_F|SDMA_CSR_RX_OWN|SDMA_CSR_RX_EI;
	first_prxp = prxp = &pmps->rx[0];
	vrxp = &vmps->rx[0];
	next_prxp = prxp + 1;
	for (n = (GTMPSC_NRXDESC - 1); n--; ) {
		dp = &vrxp->rxdesc;
		desc_write(&dp->sdma_csr, csr);
		desc_write(&dp->sdma_cnt,
			GTMPSC_RXBUFSZ << SDMA_RX_CNT_BUFSZ_SHIFT);
		desc_write(&dp->sdma_bufp, (u_int32_t)&prxp->rxbuf);
		desc_write(&dp->sdma_next, (u_int32_t)&next_prxp->rxdesc);
		GTMPSC_CACHE_FLUSH(dp);
		vrxp++;
		prxp++;
		next_prxp++;
	}
	dp = &vrxp->rxdesc;
	desc_write(&dp->sdma_csr, SDMA_CSR_RX_OWN);
	desc_write(&dp->sdma_cnt,
		GTMPSC_RXBUFSZ << SDMA_RX_CNT_BUFSZ_SHIFT);
	desc_write(&dp->sdma_bufp, (u_int32_t)&prxp->rxbuf);
	desc_write(&dp->sdma_next, (u_int32_t)&first_prxp->rxdesc);
	GTMPSC_CACHE_FLUSH(dp);
}

/*
 * Compute the BRG countdown value (CDV in BRG_BCR)
 */

STATIC int
compute_cdv(unsigned int baud)
{
	unsigned int    cdv;

	if (baud == 0)
		return 0;
	cdv = (GT_MPSC_FREQUENCY / (baud * GTMPSC_CLOCK_DIVIDER) + 1) / 2 - 1;
	if (cdv > BRG_BCR_CDV_MAX)
		return -1;
	return cdv;
}

STATIC void
gtmpsc_loadchannelregs(struct gtmpsc_softc *sc)
{
	u_int   brg_bcr;
	u_int   unit;

	unit = sc->gtmpsc_unit;
	brg_bcr = unit ? BRG_BCR1 : BRG_BCR0;

	GT_WRITE(sc, brg_bcr, sc->gtmpsc_brg_bcr);
	GT_WRITE(sc, GTMPSC_U_CHRN(unit, 3), sc->gtmpsc_chr3);
}

STATIC int
gtmpscmatch(struct device *parent, struct cfdata *self, void *aux)
{
	struct gt_softc *gt = device_private(parent);
	struct gt_attach_args *ga = aux;

	return GT_MPSCOK(gt, ga, &gtmpsc_cd);
}

STATIC void
gtmpscattach(struct device *parent, struct device *self, void *aux)
{
	struct gt_attach_args *ga = aux;
	struct gt_softc *gt = device_private(parent);
	struct gtmpsc_softc *sc = device_private(self);
	gtmpsc_poll_sdma_t *vmps;
	gtmpsc_poll_sdma_t *pmps;
	struct tty *tp;
	caddr_t kva;
	int rsegs;
	int err;
	int s;
	int is_console = 0;

	DPRINTF(("mpscattach\n"));

	GT_MPSCFOUND(gt, ga);

	s = splhigh();

	sc->gtmpsc_memt = ga->ga_memt;
	sc->gtmpsc_memh = ga->ga_memh;
	sc->gtmpsc_dmat = ga->ga_dmat;
	sc->gtmpsc_unit = ga->ga_unit;

	aprint_normal(": SDMA");
	err = bus_dmamem_alloc(sc->gtmpsc_dmat, PAGE_SIZE, PAGE_SIZE, PAGE_SIZE,
	    sc->gtmpsc_dma_segs, 1, &rsegs, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW);
	if (err) {
		PRINTF(("mpscattach: bus_dmamem_alloc error 0x%x\n", err));
		splx(s);
		return;
	}
#ifndef SDMA_COHERENT
	err = bus_dmamem_map(sc->gtmpsc_dmat, sc->gtmpsc_dma_segs, 1, PAGE_SIZE,
	    &kva, BUS_DMA_NOWAIT);
#else
	err = bus_dmamem_map(sc->gtmpsc_dmat, sc->gtmpsc_dma_segs, 1, PAGE_SIZE,
	    &kva, BUS_DMA_NOWAIT|BUS_DMA_NOCACHE);
#endif
	if (err) {
		PRINTF(("mpscattach: bus_dmamem_map error 0x%x\n", err));
		splx(s);
		return;
	}

	err = bus_dmamap_create(sc->gtmpsc_dmat, PAGE_SIZE, 1, PAGE_SIZE,
	    PAGE_SIZE, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW, &sc->gtmpsc_dma_map);
	if (err) {
		PRINTF(("mpscattach: bus_dmamap_create error 0x%x\n", err));
		splx(s);
		return;
	}

	err = bus_dmamap_load(sc->gtmpsc_dmat, sc->gtmpsc_dma_map, kva,
	    PAGE_SIZE, NULL, 0);
	if (err) {
		PRINTF(("mpscattach: bus_dmamap_load error 0x%x\n", err));
		splx(s);
		return;
	}
	memset(kva, 0, PAGE_SIZE);	/* paranoid/superfluous */

	vmps = (gtmpsc_poll_sdma_t *)kva;				    /* KVA */
	pmps = (gtmpsc_poll_sdma_t *)sc->gtmpsc_dma_map->dm_segs[0].ds_addr;    /* PA */
#if defined(DEBUG)
	printf(" at %p/%p", vmps, pmps);
#endif
	gtmpsc_txdesc_init(vmps, pmps);
	gtmpsc_rxdesc_init(vmps, pmps);
	sc->gtmpsc_poll_sdmapage = vmps;

	if (gtmpsc_scp[sc->gtmpsc_unit] != NULL)
		gtmpsc_txflush(gtmpsc_scp[sc->gtmpsc_unit]);

	sc->gtmpsc_tty = tp = ttymalloc();
	tp->t_oproc = gtmpscstart;
	tp->t_param = gtmpscparam;
	tty_attach(tp);

	if (gtmpsc_sdma_ih == NULL) {
		gtmpsc_sdma_ih = intr_establish(IRQ_SDMA, IST_LEVEL, IPL_SERIAL,
			gtmpsc_intr, &sc);
		if (gtmpsc_sdma_ih == NULL)
			panic("mpscattach: cannot intr_establish IRQ_SDMA");
	}

	sc->sc_si = softintr_establish(IPL_SOFTSERIAL, gtmpsc_softintr, sc);
	if (sc->sc_si == NULL)
		panic("mpscattach: cannot softintr_establish IPL_SOFTSERIAL");

	shutdownhook_establish(gtmpsc_shutdownhook, sc);

	gtmpsc_scp[sc->gtmpsc_unit] = sc;
	gtmpscinit(sc);

	if (cn_tab == &gtmpsc_consdev &&
	    cn_tab->cn_dev == makedev(0, sc->gtmpsc_unit)) {
		cn_tab->cn_dev = makedev(cdevsw_lookup_major(&gtmpsc_cdevsw),
		    device_unit(&sc->gtmpsc_dev));
		is_console = 1;
	}

	aprint_normal(" irq %s%s\n",
	    intr_string(IRQ_SDMA),
	    (gt_reva_gtmpsc_bug) ? " [Rev A. bug]" : "");

	if (is_console)
		aprint_normal("%s: console\n", sc->gtmpsc_dev.dv_xname);

#ifdef DDB
	if (is_console == 0)
		SDMA_IMASK_ENABLE(sc, SDMA_INTR_RXBUF(sc->gtmpsc_unit));
#endif	/* DDB */

	splx(s);
#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * the kgdb device, it has exclusive use.
	 */
	if (sc->gtmpsc_unit == comkgdbport) {
		if (comkgdbport == 0) { /* FIXME */
			printf("%s(kgdb): cannot share with console\n",
				sc->gtmpsc_dev.dv_xname);
			return;
		}

		sc->gtmpsc_flags |= GTMPSCF_KGDB;
		printf("%s: kgdb\n", sc->gtmpsc_dev.dv_xname);
		gtmpsc_txflush(gtmpsc_scp[0]);
		kgdb_attach(gtmpsc_kgdb_getc, gtmpsc_kgdb_putc, NULL);
		kgdb_dev = 123; /* unneeded, only to satisfy some tests */
		gtmpsc_kgdb_attached = 1;
		SDMA_IMASK_ENABLE(sc, SDMA_INTR_RXBUF(sc->gtmpsc_unit));
		kgdb_connect(1);
	}
#endif /* KGDB */
}

STATIC void
gtmpscshutdown(struct gtmpsc_softc *sc)
{
	struct tty *tp;
	int     s;

#ifdef KGDB
	if (sc->gtmpsc_flags & GTMPSCF_KGDB != 0)
		return;
#endif
	tp = sc->gtmpsc_tty;
	s = splserial();
	/* Fake carrier off */
	(void) (*tp->t_linesw->l_modem)(tp, 0);
	SDMA_IMASK_DISABLE(sc, SDMA_INTR_RXBUF(sc->gtmpsc_unit));
	splx(s);
}

int
gtmpscopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct gtmpsc_softc *sc;
	int unit = GTMPSCUNIT(dev);
	struct tty *tp;
	int s;
	int s2;
	int error;

	if (unit >= gtmpsc_cd.cd_ndevs)
		return ENXIO;
	sc = gtmpsc_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;
#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (sc->gtmpsc_flags & GTMPSCF_KGDB != 0)
		return (EBUSY);
#endif
	tp = sc->gtmpsc_tty;
	if (ISSET(tp->t_state, TS_ISOPEN) &&
	    ISSET(tp->t_state, TS_XCLUDE) &&
	    kauth_authorize_generic(l->l_proc->p_cred,
		 KAUTH_GENERIC_ISSUSER, &l->l_proc->p_acflag) != 0)
		return (EBUSY);

	s = spltty();

	if (!(tp->t_state & TS_ISOPEN)) {
		struct termios t;

		tp->t_dev = dev;
		s2 = splserial();
		SDMA_IMASK_ENABLE(sc, SDMA_INTR_RXBUF(unit));
		splx(s2);
		t.c_ispeed = 0;
#if 0
		t.c_ospeed = TTYDEF_SPEED;
#else
		t.c_ospeed = GT_MPSC_DEFAULT_BAUD_RATE;
#endif
		t.c_cflag = TTYDEF_CFLAG;
		/* Make sure gtmpscparam() will do something. */
		tp->t_ospeed = 0;
		(void) gtmpscparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);
		s2 = splserial();
		/* Clear the input ring */
		sc->gtmpsc_rxfifo_putix = 0;
		sc->gtmpsc_rxfifo_getix = 0;
		sc->gtmpsc_rxfifo_navail = GTMPSC_RXFIFOSZ;
		gtmpsc_iflush(sc);
		splx(s2);
	}
	splx(s);
	error = ttyopen(tp, GTMPSCDIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error)
		goto bad;

	return (0);

bad:
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * We failed to open the device, and nobody else had it opened.
		 * Clean up the state as appropriate.
		 */
		gtmpscshutdown(sc);
	}

	return (error);
}

int
gtmpscclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit = GTMPSCUNIT(dev);
	struct gtmpsc_softc *sc = gtmpsc_cd.cd_devs[unit];
	struct tty *tp = sc->gtmpsc_tty;
	int s;

	s = splserial();
	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		splx(s);
		return (0);
	}

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		gtmpscshutdown(sc);
	}

	splx(s);
	return (0);
}

int
gtmpscread(dev_t dev, struct uio *uio, int flag)
{
	struct gtmpsc_softc *sc = gtmpsc_cd.cd_devs[GTMPSCUNIT(dev)];
	struct tty *tp = sc->gtmpsc_tty;

	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
gtmpscwrite(dev_t dev, struct uio *uio, int flag)
{
	struct gtmpsc_softc *sc = gtmpsc_cd.cd_devs[GTMPSCUNIT(dev)];
	struct tty *tp = sc->gtmpsc_tty;

	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
gtmpscpoll(dev_t dev, int events, struct lwp *l)
{
	struct gtmpsc_softc *sc = gtmpsc_cd.cd_devs[GTMPSCUNIT(dev)];
	struct tty *tp = sc->gtmpsc_tty;

	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

int
gtmpscioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct gtmpsc_softc *sc = gtmpsc_cd.cd_devs[GTMPSCUNIT(dev)];
	struct tty *tp = sc->gtmpsc_tty;
	int error;

	if ((error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l)) >= 0)
		return error;
	if ((error = ttioctl(tp, cmd, data, flag, l)) >= 0)
		return error;
	return ENOTTY;
}

struct tty *
gtmpsctty(dev_t dev)
{
	struct gtmpsc_softc *sc = gtmpsc_cd.cd_devs[GTMPSCUNIT(dev)];

	return sc->gtmpsc_tty;
}

void
gtmpscstop(struct tty *tp, int flag)
{
}

STATIC void
gtmpscstart(struct tty *tp)
{
	struct gtmpsc_softc *sc;
	unsigned char *tba;
	unsigned int unit;
	int s, s2, tbc;

	unit = GTMPSCUNIT(tp->t_dev);
	sc = gtmpsc_cd.cd_devs[unit];
	if (sc == NULL)
		return;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto out;
	if (sc->sc_tx_stopped)
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if ((tp->t_state & TS_ASLEEP) != 0) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup(&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
		if (tp->t_outq.c_cc == 0)
			goto out;
	}

	/* Grab the first contiguous region of buffer space. */
	tba = tp->t_outq.c_cf;
	tbc = ndqb(&tp->t_outq, 0);

	s2 = splserial();

	sc->sc_tba = tba;
	sc->sc_tbc = tbc;
	sc->cnt_tx_from_ldisc += tbc;
	SDMA_IMASK_ENABLE(sc, SDMA_INTR_TXBUF(unit));
	tp->t_state |= TS_BUSY;
	sc->sc_tx_busy = 1;
	gtmpsc_common_putn(sc);

	splx(s2);
out:
	splx(s);
}

STATIC int
gtmpscparam(struct tty *tp, struct termios *t)
{
	struct gtmpsc_softc *sc = gtmpsc_cd.cd_devs[GTMPSCUNIT(tp->t_dev)];
	int ospeed = compute_cdv(t->c_ospeed);
	int s;

	/* Check requested parameters. */
	if (ospeed < 0)
		return (EINVAL);
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return (EINVAL);

	/*
	 * If there were no changes, don't do anything.  This avoids dropping
	 * input and improves performance when all we did was frob things like
	 * VMIN and VTIME.
	 */
	if (tp->t_ospeed == t->c_ospeed &&
	    tp->t_cflag == t->c_cflag)
		return (0);

	s = splserial();

	sc->gtmpsc_brg_bcr = BRG_BCR_EN | GT_MPSC_CLOCK_SOURCE | ospeed;
	sc->gtmpsc_chr3 = GTMPSC_MAXIDLE(t->c_ospeed);

	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			gtmpsc_loadchannelregs(sc);
	}

	splx(s);

	/* Fake carrier on */
	(void) (*tp->t_linesw->l_modem)(tp, 1);

	return 0;
}

STATIC int
gtmpsc_probe(void)
{
	return 1;		/* XXX */
}

STATIC unsigned int
gtmpsc_get_causes(void)
{
	int                     i;
	struct gtmpsc_softc       *sc;
	unsigned int            cause = 0;
	static unsigned int     bits[4] = {
				SDMA_INTR_RXBUF(0),
				SDMA_INTR_TXBUF(0),
				SDMA_INTR_RXBUF(1),
				SDMA_INTR_TXBUF(1),
	};
	sdma_desc_t             *desc_addr[4];
	static unsigned int     fake_once = SDMA_INTR_RXBUF(0)
						| SDMA_INTR_RXBUF(1);

	desc_addr[0] = 0;
	desc_addr[1] = 0;
	desc_addr[2] = 0;
	desc_addr[3] = 0;
	sc = gtmpsc_cd.cd_devs[0];
	if (sc != 0) {
	    if (sdma_imask & SDMA_INTR_RXBUF(0)) {
		desc_addr[0] =
		    &sc->gtmpsc_poll_sdmapage->rx[sc->gtmpsc_poll_rxix].rxdesc;
		    GTMPSC_CACHE_INVALIDATE(desc_addr[0]);
		    __asm volatile ("dcbt 0,%0" :: "r"(desc_addr[0]));
	    }
	    if (sdma_imask & SDMA_INTR_TXBUF(0)) {
		desc_addr[1] =
		    &sc->gtmpsc_poll_sdmapage->tx[sc->gtmpsc_poll_txix].txdesc;
		    GTMPSC_CACHE_INVALIDATE(desc_addr[1]);
		    __asm volatile ("dcbt 0,%0" :: "r"(desc_addr[1]));
	    }
	}
	sc = gtmpsc_cd.cd_devs[1];
	if (sc != 0) {
	    if (sdma_imask & SDMA_INTR_RXBUF(1)) {
		desc_addr[2] =
		    &sc->gtmpsc_poll_sdmapage->rx[sc->gtmpsc_poll_rxix].rxdesc;
		    GTMPSC_CACHE_INVALIDATE(desc_addr[2]);
		    __asm volatile ("dcbt 0,%0" :: "r"(desc_addr[2]));
	    }
	    if (sdma_imask & SDMA_INTR_TXBUF(1)) {
		desc_addr[3] =
		    &sc->gtmpsc_poll_sdmapage->tx[sc->gtmpsc_poll_txix].txdesc;
		    GTMPSC_CACHE_INVALIDATE(desc_addr[3]);
		    __asm volatile ("dcbt 0,%0" :: "r"(desc_addr[3]));
	    }
	}

	for (i = 0; i < 4; ++i)
		if ((sdma_imask & bits[i]) && desc_addr[i] != 0 &&
			    (desc_addr[i]->sdma_csr & SDMA_CSR_TX_OWN) == 0)
			cause |= bits[i];
	if (fake_once & sdma_imask) {
		cause |= fake_once & sdma_imask;
	/*	fake_once &= ~(cause & fake_once); */
	}
	return cause;
}

STATIC int
gtmpsc_intr(void *arg)
{
	struct gtmpsc_softc       *sc;
	unsigned int            unit;
	int                     spurious = 1;
	unsigned int            r;
	unsigned int            cause=0;

	if (gt_reva_gtmpsc_bug)
		cause = gtmpsc_get_causes();

#ifdef KGDB
	if (kgdb_break_immediate) {
		unit = comkgdbport;
		sc = gtmpsc_cd.cd_devs[unit];
		if (sc == 0 || (sc->gtmpsc_flags & GTMPSCF_KGDB) == 0)
			goto skip_kgdb;
		if (gt_reva_gtmpsc_bug)
			r = cause & sdma_imask;
		else {
			r = GT_READ(sc, SDMA_ICAUSE);
			r &= GT_READ(sc, SDMA_IMASK);
		}
		r &= SDMA_INTR_RXBUF(unit);
		if (r == 0)
			goto skip_kgdb;
		GT_WRITE(sc, SDMA_ICAUSE, ~r);
		spurious = 0;
		gtmpsc_kgdb_poll(sc);
	}
skip_kgdb:
#endif
	for (unit = 0; unit < GTMPSC_NCHAN; ++unit) {
		sc = gtmpsc_cd.cd_devs[unit];
		if (sc == 0)
			continue;
		if (gt_reva_gtmpsc_bug)
			r = cause & sdma_imask;
		else {
			r = GT_READ(sc, SDMA_ICAUSE);
			r &= GT_READ(sc, SDMA_IMASK);
		}
		r &= SDMA_U_INTR_MASK(unit);
		if (r == 0)
			continue;
		GT_WRITE(sc, SDMA_ICAUSE, ~r);
		spurious = 0;
		if (r & SDMA_INTR_RXBUF(unit)) {
#ifdef KGDB
			if (sc->gtmpsc_flags & GTMPSCF_KGDB)
				gtmpsc_kgdb_poll(sc);
			else
#endif
				gtmpsc_poll(sc);
		}
		if (r & SDMA_INTR_TXBUF(unit)) {
			/*
			 * If we've delayed a parameter change, do it now,
			 * and restart output.
			 */
			if (sc->sc_heldchange) {
				gtmpsc_loadchannelregs(sc);
				sc->sc_heldchange = 0;
				sc->sc_tbc = sc->sc_heldtbc;
				sc->sc_heldtbc = 0;
			}

			/* Output the next chunk of the contiguous buffer,
								if any. */
			if (sc->sc_tbc > 0)
				gtmpsc_common_putn(sc);
			if (sc->sc_tbc == 0 && sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				sc->sc_tx_done = 1;
				softintr_schedule(sc->sc_si);
				SDMA_IMASK_DISABLE(sc, SDMA_INTR_TXBUF(unit));
			}
		}
	}
	return 1;
	/* return !spurious; */
}

STATIC void
gtmpsc_softintr(void *arg)
{
	struct gtmpsc_softc	*sc = arg;
	struct tty              *tp;
	int                     (*rint)(int, struct tty *);
	int                     jobs;
	int                     s;

	tp = sc->gtmpsc_tty;
	rint = tp->t_linesw->l_rint;
	do {
		jobs = 0;
		if (sc->gtmpsc_rxfifo_navail < GTMPSC_RXFIFOSZ) {
			s = spltty();
			rint(sc->gtmpsc_rxfifo[sc->gtmpsc_rxfifo_getix++],
								tp);
			if (sc->gtmpsc_rxfifo_getix >= GTMPSC_RXFIFOSZ)
				sc->gtmpsc_rxfifo_getix = 0;
			++sc->cnt_rx_from_fifo;
			/* atomic_add() returns the previous value */
			jobs += atomic_add(&sc->gtmpsc_rxfifo_navail, 1) + 1
								< GTMPSC_RXFIFOSZ;
			splx(s);
		}
		if (sc->sc_tx_done) {
			++jobs;
			sc->sc_tx_done = 0;
			s = spltty();
			tp->t_state &= ~TS_BUSY;
			if ((tp->t_state & TS_FLUSH) != 0)
			    tp->t_state &= ~TS_FLUSH;
			else
			    ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
			(*tp->t_linesw->l_start)(tp);
			splx(s);
		}
	} while (jobs);
}

/*
 * Console support functions
 */
void
gtmpsccnprobe(struct consdev *cd)
{
	int maj;
/* {extern void return_to_dink(int); return_to_dink(gtbase);} */

	if (!gtmpsc_probe())
		return;

	maj = cdevsw_lookup_major(&gtmpsc_cdevsw);
	cd->cn_dev = makedev(maj, 0);
	cd->cn_pri = CN_INTERNAL;
}

/*
 * gtmpsc_hackinit - hacks required to supprt GTMPSC console
 */
STATIC void
gtmpsc_hackinit(struct gtmpsc_softc *sc, bus_space_tag_t memt,
	bus_space_handle_t memh, int unit)
{
	gtmpsc_poll_sdma_t *vmps;
	gtmpsc_poll_sdma_t *pmps;

	DPRINTF(("hackinit\n"));

	bzero(sc, sizeof(struct gtmpsc_softc));
	sc->gtmpsc_memt = memt;
	sc->gtmpsc_memh = memh;
	sc->gtmpsc_unit = unit;
	gtmpsc_scp[sc->gtmpsc_unit] = sc;

	vmps = (gtmpsc_poll_sdma_t *)gtmpsc_fake_dmapage;	/* KVA */
	pmps = (gtmpsc_poll_sdma_t *)gtmpsc_fake_dmapage;	/* PA */

	gtmpsc_txdesc_init(vmps, pmps);
	gtmpsc_rxdesc_init(vmps, pmps);

	sc->gtmpsc_poll_sdmapage = vmps;
}

/*
 * gtmpsc_txflush - wait for output to drain
 */
STATIC void
gtmpsc_txflush(gtmpsc_softc_t *sc)
{
	unsigned int csr;
	unsigned int *csrp;
	gtmpsc_polltx_t *vtxp;
	int limit = 4000000;	/* 4 seconds */
	int ix;

	ix = sc->gtmpsc_poll_txix - 1;
	if (ix < 0)
		ix = GTMPSC_NTXDESC - 1;

	vtxp = &sc->gtmpsc_poll_sdmapage->tx[ix];
	csrp = &vtxp->txdesc.sdma_csr;
	while (limit > 0) {
		GTMPSC_CACHE_INVALIDATE(csrp);
		csr = desc_read(csrp);
		if ((csr & SDMA_CSR_TX_OWN) == 0)
			break;
		DELAY(GTMPSC_POLL_DELAY);
		limit -= GTMPSC_POLL_DELAY;
	}
}

STATIC void
gtmpsc_iflush(gtmpsc_softc_t *sc)
{
	int     timo;
	char    c;
	int     stat;

	for (timo = 50000; timo; timo--)
		if (gtmpsc_common_pollc(sc->gtmpsc_unit, &c, &stat) == 0)
			return;
#ifdef DIAGNOSTIC
	printf("%s: gtmpsc_iflush timeout %02x\n", sc->gtmpsc_dev.dv_xname, c);
#endif
}

STATIC void
gtmpscinit_stop(struct gtmpsc_softc *sc, int once)
{
	unsigned int r;
	unsigned int unit = sc->gtmpsc_unit;

	/*
	 * XXX HACK FIXME
	 * PMON output has not been flushed.  give him a chance
	 */
#if 1
	if (! once)
		DELAY(100000);	/* XXX */
#endif

	DPRINTF(("gtmpscinit_stop: unit 0x%x\n", unit));
	if (unit >= GTMPSC_NCHAN) {
		PRINTF(("mpscinit: undefined unit %d\n", sc->gtmpsc_unit));
		return;
	}

	sc->gtmpsc_chr2 = 0;      /* Default value of CHR2 */

	/*
	 * stop GTMPSC unit
	 */
	r = sc->gtmpsc_chr2 | GTMPSC_CHR2_RXABORT|GTMPSC_CHR2_TXABORT;
	GT_WRITE(sc, GTMPSC_U_CHRN(unit, 2), r);

	DELAY(GTMPSC_RESET_DELAY);

	/*
	 * abort SDMA TX, RX for GTMPSC unit
	 */
	GT_WRITE(sc, SDMA_U_SDCM(unit), SDMA_SDCM_AR|SDMA_SDCM_AT);

	if (once == 0) {
		/*
		 * Determine if this is the buggy GT-64260A case.
		 * If this is, then most of GTMPSC and SDMA registers
		 * are unreadable.
		 * (They always yield -1).
		 */
		GT_WRITE(sc, SDMA_IMASK, 0);
		r = GT_READ(sc, SDMA_IMASK);
		gt_reva_gtmpsc_bug = r == ~0;
		sdma_imask = 0;
	}

	/*
	 * If Rx is disabled, we don't need to wait around for
	 * abort completion.
	 */
	if ((GT_READ(sc, GTMPSC_U_MMCR_LO(unit)) & GTMPSC_MMCR_LO_ER) == 0)
		return;

	/*
	 * poll for GTMPSC RX abort completion
	 */
	if (gt_reva_gtmpsc_bug) {
		/* Sync up with the device first */
		r = GT_READ(sc, GTMPSC_U_CHRN(unit, 2));
		DELAY(GTMPSC_RESET_DELAY);
	} else
		for (;;) {
			r = GT_READ(sc, GTMPSC_U_CHRN(unit, 2));
			if (! (r & GTMPSC_CHR2_RXABORT))
				break;
		}

	/*
	 * poll for SDMA RX abort completion
	 */
	for (;;) {
		r = GT_READ(sc, SDMA_U_SDCM(unit));
		if (! (r & (SDMA_SDCM_AR|SDMA_SDCM_AT)))
			break;
		DELAY(50);
	}

}

STATIC void
gtmpscinit_start(struct gtmpsc_softc *sc, int once)
{
	unsigned int r;
	unsigned int unit = sc->gtmpsc_unit;

	/*
	 * initialize softc's "current" transfer indicies & counts
	 */
	sc->gtmpsc_cx = 0;
	sc->gtmpsc_nc = 0;
	sc->gtmpsc_poll_txix = 0;
	sc->gtmpsc_poll_rxix = 0;

	/*
	 * initialize softc's RX softintr FIFO
	 */
	sc->gtmpsc_rxfifo_putix = 0;
	sc->gtmpsc_rxfifo_getix = 0;
	sc->gtmpsc_rxfifo_navail = GTMPSC_RXFIFOSZ;
	memset(&sc->gtmpsc_rxfifo[0], 0, GTMPSC_RXFIFOSZ);

	/*
	 * set SDMA unit port TX descriptor pointers
	 * "next" pointer of last descriptor is start of ring
	 */
	r = desc_read(
	    &sc->gtmpsc_poll_sdmapage->tx[GTMPSC_NTXDESC-1].txdesc.sdma_next);
	GT_WRITE(sc, SDMA_U_SCTDP(unit), r);   /* current */
	(void)GT_READ(sc, SDMA_U_SCTDP(unit));
	GT_WRITE(sc, SDMA_U_SFTDP(unit), r);   /* first   */
	(void)GT_READ(sc, SDMA_U_SFTDP(unit));
	/*
	 * set SDMA unit port RX descriptor pointer
	 * "next" pointer of last descriptor is start of ring
	 */
	r = desc_read(
	    &sc->gtmpsc_poll_sdmapage->rx[GTMPSC_NRXDESC-1].rxdesc.sdma_next);
	GT_WRITE(sc, SDMA_U_SCRDP(unit), r);   /* current */

	/*
	 * initialize SDMA unit Configuration Register
	 */
	r = SDMA_SDC_BSZ_8x64|SDMA_SDC_SFM|SDMA_SDC_RFT;
	GT_WRITE(sc, SDMA_U_SDC(unit), r);

	/*
	 * enable SDMA receive
	 */
	GT_WRITE(sc, SDMA_U_SDCM(unit), SDMA_SDCM_ERD);

	if (once == 0) {
		/*
		 * GTMPSC Routing:
		 *	MR0 --> Serial Port 0
		 *	MR1 --> Serial Port 1
		 */
		GT_WRITE(sc, GTMPSC_MRR, GTMPSC_MRR_RES);

		/*
		 * RX and TX Clock Routing:
		 *      CRR0 --> BRG0
		 *      CRR1 --> BRG1
		 */
		r = GTMPSC_CRR_BRG0 | (GTMPSC_CRR_BRG1 << GTMPSC_CRR1_SHIFT);
		GT_WRITE(sc, GTMPSC_RCRR, r);
		GT_WRITE(sc, GTMPSC_TCRR, r);
	}
	sc->gtmpsc_brg_bcr = BRG_BCR_EN | GT_MPSC_CLOCK_SOURCE |
	    compute_cdv(GT_MPSC_DEFAULT_BAUD_RATE);
	sc->gtmpsc_chr3 = GTMPSC_MAXIDLE(GT_MPSC_DEFAULT_BAUD_RATE);
	gtmpsc_loadchannelregs(sc);

	/*
	 * set MPSC Protocol configuration register for GTMPSC unit
	 */
	GT_WRITE(sc, GTMPSC_U_MPCR(unit), GTMPSC_MPCR_CL_8);

	/*
	 * set MPSC LO and HI port config registers for GTMPSC unit
 	 */
	r = GTMPSC_MMCR_LO_MODE_UART
	   |GTMPSC_MMCR_LO_ET
	   |GTMPSC_MMCR_LO_ER
	   |GTMPSC_MMCR_LO_NLM;
	GT_WRITE(sc, GTMPSC_U_MMCR_LO(unit), r);

	r =
	    GTMPSC_MMCR_HI_TCDV_DEFAULT
	   |GTMPSC_MMCR_HI_RDW
	   |GTMPSC_MMCR_HI_RCDV_DEFAULT;
	GT_WRITE(sc, GTMPSC_U_MMCR_HI(unit), r);

	/*
	 * tell MPSC receive the Enter Hunt
	 */
	r = sc->gtmpsc_chr2 | GTMPSC_CHR2_EH;
	GT_WRITE(sc, GTMPSC_U_CHRN(unit, 2), r);

	/*
	 * clear any pending SDMA interrupts for this unit
	 */
	r = GT_READ(sc, SDMA_ICAUSE);
	r &= ~SDMA_U_INTR_MASK(unit);
	GT_WRITE(sc, SDMA_ICAUSE, r);	/* ??? */

	DPRINTF(("gtmpscinit: OK\n"));
}

/*
 * gtmpscinit - prepare MPSC for operation
 *
 *	assumes we are called at ipl >= IPL_SERIAL
 */
STATIC void
gtmpscinit(struct gtmpsc_softc *sc)
{
	static int once = 0;

	gtmpscinit_stop(sc, once);
	gtmpscinit_start(sc, once);
	once = 1;
}

/*
 * gtmpsccninit - initialize the driver and the mpsc device
 */
void
gtmpsccninit(struct consdev *cd)
{
}

int
gtmpsccngetc(dev_t dev)
{
	unsigned int unit = 0;
	int c;

	unit = GTMPSCUNIT(dev);
	if (major(dev) != 0) {
		struct gtmpsc_softc *sc = device_lookup(&gtmpsc_cd, unit);
		if (sc == NULL)
			return 0;
		unit = sc->gtmpsc_unit;
	}
	if (unit >= GTMPSC_NCHAN)
		return 0;
	c = gtmpsc_common_getc(unit);

	return c;
}

void
gtmpsccnputc(dev_t dev, int c)
{
	unsigned int unit = 0;
	char ch = c;
	static int ix = 0;

	if (gtmpsccninit_done == 0) {
		if ((minor(dev) == 0) && (ix < sizeof(gtmpsc_earlybuf)))
			gtmpsc_earlybuf[ix++] = (unsigned char)c;
		return;
	}

	unit = GTMPSCUNIT(dev);
	if (major(dev) != 0) {
		struct gtmpsc_softc *sc = device_lookup(&gtmpsc_cd, unit);
		if (sc == NULL)
			return;
		unit = sc->gtmpsc_unit;
	}

	if (unit >= GTMPSC_NCHAN)
		return;

	gtmpsc_common_putc(unit, ch);
}

void
gtmpsccnpollc(dev_t dev, int on)
{
}

int
gtmpsccnattach(bus_space_tag_t memt, bus_space_handle_t memh, int unit,
	int speed, tcflag_t tcflag)
{
	struct gtmpsc_softc *sc = &gtmpsc_fake_softc;
	unsigned char *cp;
	unsigned char c;
	unsigned int i;

	if (gtmpsccninit_done)
		return 0;

	DPRINTF(("gtmpsccnattach\n"));
	gtmpsc_hackinit(sc, memt, memh, unit);
	DPRINTF(("gtmpscinit\n"));
	gtmpscinit(sc);
	gtmpsccninit_done = 1;
	cp = gtmpsc_earlybuf;
	strcpy(gtmpsc_earlybuf, "\r\nMPSC Lives!\r\n");
	for (i=0; i < sizeof(gtmpsc_earlybuf); i++) {
		c = *cp++;
		if (c == 0)
			break;
		gtmpsc_common_putc(unit, c);
	}
	DPRINTF(("switching cn_tab\n"));
	gtmpsc_consdev.cn_dev = makedev(0, unit);
	cn_tab = &gtmpsc_consdev;
	DPRINTF(("switched cn_tab!\n"));
	return 0;
}

/*
 * gtmpsc_common_pollc - non-blocking console read
 *
 *	if there is an RX char, return it in *cp
 *	set *statp if Break detected
 *
 *	assumes we are called at ipl >= IPL_SERIAL
 *
 * return 1 if there is RX data
 * otherwise return 0
 */
STATIC int
gtmpsc_common_pollc(unsigned int unit, char *cp, int *statp)
{
	struct gtmpsc_softc *sc = gtmpsc_scp[unit];
	gtmpsc_pollrx_t *vrxp;
	unsigned int ix;
	unsigned int cx;
	unsigned int nc;

	*statp = GTMPSC_STAT_NONE;
	ix = sc->gtmpsc_poll_rxix;
	nc = sc->gtmpsc_nc;
	cx = sc->gtmpsc_cx;
	if (nc == 0) {
		unsigned int *csrp;
		unsigned int csr;

		vrxp = &sc->gtmpsc_poll_sdmapage->rx[ix];
		csrp = &vrxp->rxdesc.sdma_csr;
		cx = 0;

		GTMPSC_CACHE_INVALIDATE(csrp);
		csr = desc_read(csrp);
		if (csr & SDMA_CSR_RX_OWN)
			return 0;
		if (csr & SDMA_CSR_RX_BR)
			*statp = GTMPSC_STAT_BREAK;
		if (csr & SDMA_CSR_RX_ES)
			PRINTF(("mpsc 0 RX error, rxdesc csr 0x%x\n", csr));

		nc = desc_read(&vrxp->rxdesc.sdma_cnt);
		nc &= SDMA_RX_CNT_BCNT_MASK;
		csr = SDMA_CSR_RX_L|SDMA_CSR_RX_F|SDMA_CSR_RX_OWN
							      |SDMA_CSR_RX_EI;
		if (nc == 0) {
			if ((++ix) >= GTMPSC_NRXDESC)
				ix = 0;
			sc->gtmpsc_poll_rxix = ix;
			desc_write(csrp, csr);
			GTMPSC_CACHE_FLUSH(csrp);
			return 0;
		}
		bcopy(vrxp->rxbuf, sc->gtmpsc_rxbuf, nc);
		desc_write(csrp, csr);
		GTMPSC_CACHE_FLUSH(csrp);
	}
	gtmpsc_poll_pollc_cnt++;
	nc--;
	*cp = sc->gtmpsc_rxbuf[cx++];
	if (nc == 0) {
		if ((++ix) >= GTMPSC_NRXDESC)
			ix = 0;
		sc->gtmpsc_poll_rxix = ix;
	}
	sc->gtmpsc_cx = cx;
	sc->gtmpsc_nc = nc;
	return 1;
}

/*
 * gtmpsc_common_getc - polled console read
 *
 *	We copy data from the DMA buffers into a buffer in the softc
 *	to reduce descriptor ownership turnaround time
 *	MPSC can crater if it wraps descriptor rings,
 *	which is asynchronous and throttled only by line speed.
 *
 *	This code assumes the buffer PA==KVA
 *	and assumes we are called at ipl >= IPL_SERIAL
 */
STATIC int
gtmpsc_common_getc(unsigned int unit)
{
	struct gtmpsc_softc *sc = gtmpsc_scp[unit];
	gtmpsc_pollrx_t *vrxp;
	unsigned int ix;
	unsigned int cx;
	unsigned int nc;
	unsigned int wdog_interval;
	int c;

	ix = sc->gtmpsc_poll_rxix;
	nc = sc->gtmpsc_nc;
	cx = sc->gtmpsc_cx;
	wdog_interval = 0;
	while (nc == 0) {
		unsigned int *csrp;
		unsigned int csr;
		unsigned int r;

		vrxp = &sc->gtmpsc_poll_sdmapage->rx[ix];
		csrp = &vrxp->rxdesc.sdma_csr;
		cx = 0;

		GTMPSC_CACHE_INVALIDATE(csrp);
		csr = desc_read(csrp);
		if (csr & SDMA_CSR_RX_OWN) {
			r = sc->gtmpsc_chr2 | GTMPSC_CHR2_CRD;
			GT_WRITE(sc, GTMPSC_U_CHRN(unit, 2), r);
			do {
				if (wdog_interval++ % 32)
					gt_watchdog_service();
				gtmpsc_poll_getc_miss++;
				GTMPSC_CACHE_INVALIDATE(csrp);
				DELAY(50);
				csr = desc_read(csrp);
			} while (csr & SDMA_CSR_RX_OWN);
		} else
		if (wdog_interval++ % 32)
			gt_watchdog_service();
		if (csr & SDMA_CSR_RX_ES)
			PRINTF(("mpsc 0 RX error, rxdesc csr 0x%x\n", csr));

		nc = desc_read(&vrxp->rxdesc.sdma_cnt);
		nc &= SDMA_RX_CNT_BCNT_MASK;
		if (nc) {
			bcopy(vrxp->rxbuf, sc->gtmpsc_rxbuf, nc);
		} else {
			if ((++ix) >= GTMPSC_NRXDESC)
				ix = 0;
			sc->gtmpsc_poll_rxix = ix;
		}
		csr = SDMA_CSR_RX_L|SDMA_CSR_RX_F|SDMA_CSR_RX_OWN
							|SDMA_CSR_RX_EI;
		desc_write(csrp, csr);
		GTMPSC_CACHE_FLUSH(csrp);
#ifdef KGDB
		if (unit == comkgdbport && gt_reva_gtmpsc_bug)
			GT_WRITE(sc, SDMA_ICAUSE, ~SDMA_INTR_RXBUF(unit));
#endif
	}
	gtmpsc_poll_getc_cnt++;
	nc--;
	c = (int)sc->gtmpsc_rxbuf[cx++];
	if (nc == 0) {
		if ((++ix) >= GTMPSC_NRXDESC)
			ix = 0;
		sc->gtmpsc_poll_rxix = ix;
	}
	sc->gtmpsc_cx = cx;
	sc->gtmpsc_nc = nc;
	gt_watchdog_service();
	return c;
}

/*
 * gtmpsc_common_putn - write a buffer into the hardware
 *
 *	assumes we are called at ipl >= IPL_SERIAL
 */
STATIC void
gtmpsc_common_putn(struct gtmpsc_softc *sc)

{
	int     unit = sc->gtmpsc_unit;
	int     n;
	int     kick;
	gtmpsc_polltx_t *vtxp;
	unsigned int *csrp;
	unsigned int csr;
	unsigned int ix;
	unsigned int sdcm;

	kick = 0;
	for (ix = sc->gtmpsc_poll_txix; sc->sc_tbc;) {
		vtxp = &sc->gtmpsc_poll_sdmapage->tx[ix];
		csrp = &vtxp->txdesc.sdma_csr;
		GTMPSC_CACHE_INVALIDATE(csrp);
		csr = desc_read(csrp);
		if ((csr & SDMA_CSR_TX_OWN) != 0)
			break;
		n = sc->sc_tbc;
		if (n > GTMPSC_TXBUFSZ)
			n = GTMPSC_TXBUFSZ;
		bcopy(sc->sc_tba, vtxp->txbuf, n);
		csr = SDMA_CSR_TX_L | SDMA_CSR_TX_F
					| SDMA_CSR_TX_EI | SDMA_CSR_TX_OWN;
		desc_write(&vtxp->txdesc.sdma_cnt,
				(n << SDMA_TX_CNT_BCNT_SHIFT) | n);
		desc_write(csrp, csr);
		GTMPSC_CACHE_FLUSH(csrp);
		sc->sc_tbc -= n;
		sc->sc_tba += n;
		gtmpsc_poll_putn_cnt += n;
		sc->cnt_tx_to_sdma += n;
		kick = 1;

		if (++ix >= GTMPSC_NTXDESC)
			ix = 0;
	}
	if (kick) {
		sc->gtmpsc_poll_txix = ix;

		/*
		 * now kick some SDMA
		 */
		sdcm = GT_READ(sc, SDMA_U_SDCM(unit));

		if ((sdcm & SDMA_SDCM_TXD) == 0) {
			GT_WRITE(sc, SDMA_U_SDCM(unit), SDMA_SDCM_TXD);
		}
	}
}

/*
 * gtmpsc_common_putc - polled console putc
 *
 *	assumes we are called at ipl >= IPL_SERIAL
 */
STATIC void
gtmpsc_common_putc(unsigned int unit, unsigned char c)
{
	struct gtmpsc_softc *sc = gtmpsc_scp[unit];
	gtmpsc_polltx_t *vtxp;
	unsigned int *csrp;
	unsigned int csr;
	unsigned int ix;
	unsigned int nix;
	unsigned int wdog_interval = 0;

	DPRINTF(("gtmpsc_common_putc(%d, '%c'): cur=%#x, first=%#x", unit, c,
	    GT_READ(sc, SDMA_U_SCTDP(unit)),   /* current */
	    GT_READ(sc, SDMA_U_SFTDP(unit))));   /* first   */
	ix = sc->gtmpsc_poll_txix;
	nix = ix + 1;
	if (nix >= GTMPSC_NTXDESC)
		nix = 0;
	sc->gtmpsc_poll_txix = nix;
	vtxp = &sc->gtmpsc_poll_sdmapage->tx[ix];
	csrp = &vtxp->txdesc.sdma_csr;


	for (;;) {
		GTMPSC_CACHE_INVALIDATE(csrp);
		csr = desc_read(csrp);
		if ((csr & SDMA_CSR_TX_OWN) == 0)
			break;
		gtmpsc_poll_putc_miss++;
		DELAY(40);
		DPRINTF(("."));
		if (wdog_interval++ % 32)
			gt_watchdog_service();
	}
	if (csr & SDMA_CSR_TX_ES)
		PRINTF(("mpsc %d TX error, txdesc csr 0x%x\n", unit, csr));

	gtmpsc_poll_putc_cnt++;
	vtxp->txbuf[0] = c;
	csr = SDMA_CSR_TX_L | SDMA_CSR_TX_F | SDMA_CSR_TX_EI | SDMA_CSR_TX_OWN;
	desc_write(&vtxp->txdesc.sdma_cnt, (1 << SDMA_TX_CNT_BCNT_SHIFT) | 1);
	desc_write(csrp, csr);
	GTMPSC_CACHE_FLUSH(csrp);

	/*
	 * now kick some SDMA
	 */
	GT_WRITE(sc, SDMA_U_SDCM(unit), SDMA_SDCM_TXD);
	gt_watchdog_service();
}


STATIC void
gtmpsc_poll(void *arg)
{
	struct gtmpsc_softc *sc = (struct gtmpsc_softc *)arg;
	int kick;
	char ch;
	int stat;
	static struct timeval   msg_time = {0,0};
	static struct timeval   cur_time;
	static int fifo_full = 0;

	kick = 0;
	while (gtmpsc_common_pollc(sc->gtmpsc_unit, &ch, &stat)) {
#ifdef DDB
		if (stat)
			break;
#endif
		++sc->cnt_rx_from_sdma;
		if (sc->gtmpsc_rxfifo_navail != 0) {
			sc->gtmpsc_rxfifo[sc->gtmpsc_rxfifo_putix++] = ch;
			if (sc->gtmpsc_rxfifo_putix == GTMPSC_RXFIFOSZ)
				sc->gtmpsc_rxfifo_putix = 0;
			atomic_add(&sc->gtmpsc_rxfifo_navail, -1);
			++sc->cnt_rx_to_fifo;
			fifo_full = 0;
			kick = 1;
		} else {
			if (fifo_full == 0) {
				fifo_full = 1;
				microtime(&cur_time);
				if (cur_time.tv_sec - msg_time.tv_sec >= 5) {
					/* Only do this once in 5 sec */
					msg_time = cur_time;
					printf("mpsc%d: input FIFO full, "
					       "dropping incoming characters\n",
					       sc->gtmpsc_unit);
				}
			}
		}
	}
#ifdef DDB
	if (stat) {
		if (cn_tab == &gtmpsc_consdev) {
			Debugger();
		}
	}
#endif
	if (kick)
		softintr_schedule(sc->sc_si);
}

#ifdef KGDB
/* ARGSUSED */
STATIC int
gtmpsc_kgdb_getc(arg)
	void *arg;
{

	return (gtmpsc_common_getc(comkgdbport));
}

/* ARGSUSED */
STATIC void
gtmpsc_kgdb_putc(arg, c)
	void *arg;
	int c;
{

	return (gtmpsc_common_putc(comkgdbport, c));
}

STATIC void
gtmpsc_kgdb_poll(void *arg)
{
	struct gtmpsc_softc       *sc = (struct gtmpsc_softc *)arg;
	int                     s;
	char                    c;
	int                     brk;

	s = splserial();
	if (kgdb_recover == 0) { /* gdb is not currently talking to its agent */
		while (gtmpsc_common_pollc(sc->gtmpsc_unit, &c, &brk)) {
			if (c == CTRL('c'))
				brk = GTMPSC_STAT_BREAK;
			if (brk == GTMPSC_STAT_BREAK)
				break;
		}
		if (brk == GTMPSC_STAT_BREAK) {
			if (kgdb_break_immediate)
				breakpoint();
			else {
				printf("connecting to kgdb\n");
				kgdb_connect(1);
			}
		}
	}
	splx(s);
}

#endif /* KGDB */

#if 0
void
gtmpsc_printf(const char *fmt, ...)
{
	struct consdev *ocd;
	int s;
	va_list ap;

	s = splserial();
	ocd = cn_tab;
	cn_tab = &constab[0];
	va_start(ap, fmt);
	printf(fmt, ap);
	va_end(ap);
	cn_tab = ocd;
	splx(s);
}
#endif

void
gtmpsc_mem_printf(const char *fmt, ...)
{
	va_list ap;
	static unsigned char *p = gtmpsc_print_buf;

	if (p >= &gtmpsc_print_buf[GTMPSC_PRINT_BUF_SIZE - 128]) {
		bzero(gtmpsc_print_buf, GTMPSC_PRINT_BUF_SIZE);
		p = gtmpsc_print_buf;
	}
	va_start(ap, fmt);
	p += vsprintf(p, fmt, ap);
	va_end(ap);
}

void
gtmpsc_shutdownhook(void *arg)
{
	gtmpsc_softc_t *sc = (gtmpsc_softc_t *)arg;

	gtmpsc_txflush(sc);
}

void
gtmpsccnhalt(dev_t dev)
{
	unsigned int unit;
	u_int32_t r;

	for (unit = 0; unit < GTMPSC_NCHAN; unit++) {
		gtmpsc_softc_t *sc = gtmpsc_scp[unit];
		if (sc == NULL)
			continue;

		/*
		 * flush TX buffers
		 */
		gtmpsc_txflush(sc);

		/*
		 * stop MPSC unit RX
		 */
		r = GT_READ(sc, GTMPSC_U_CHRN(unit, 2));
		r &= ~GTMPSC_CHR2_EH;
		r |= GTMPSC_CHR2_RXABORT;
		GT_WRITE(sc, GTMPSC_U_CHRN(unit, 2), r);

		DELAY(GTMPSC_RESET_DELAY);

		/*
		 * abort SDMA RX for MPSC unit
		 */
		GT_WRITE(sc, SDMA_U_SDCM(unit), SDMA_SDCM_AR);
	}
}

void
gtmpsc_puts(char *str)
{
	char c;

	if (str == NULL)
		return;
	while ((c = *str++) != 0)
		gtmpsccnputc(0, c);
}
