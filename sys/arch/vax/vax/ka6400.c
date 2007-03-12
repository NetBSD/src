/*	$NetBSD: ka6400.c,v 1.10.2.1 2007/03/12 05:51:17 rmind Exp $	*/

/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden. All rights reserved.
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
 *      This product includes software developed at Ludd, University of
 *      Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * KA6400 specific CPU code.
 */
/*
 * TODO:
 *	- Machine check code
 *	- Vector processor code
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ka6400.c,v 1.10.2.1 2007/03/12 05:51:17 rmind Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <machine/ka670.h>
#include <machine/cpu.h>
#include <machine/mtpr.h>
#include <machine/nexus.h>
#include <machine/clock.h>
#include <machine/scb.h>
#include <machine/bus.h>
#include <machine/sid.h>
#include <machine/cca.h>
#include <machine/rpb.h>

#include <dev/xmi/xmireg.h>
#include <dev/xmi/xmivar.h>

#include "ioconf.h"
#include "locators.h"

static int *rssc;
struct cca *cca;

static int ka6400_match(struct device *, struct cfdata *, void *);
static void ka6400_attach(struct device *, struct device *, void*);
static void ka6400_memerr(void);
static void ka6400_conf(void);
static int ka6400_mchk(void *);
static void ka6400_steal_pages(void);
#if defined(MULTIPROCESSOR)
static void ka6400_startslave(struct device *, struct cpu_info *);
static void ka6400_txrx(int, const char *, int);
static void ka6400_sendstr(int, const char *);
static void ka6400_sergeant(int);
static int rxchar(void);
static void ka6400_putc(int);
static void ka6400_cnintr(void);
cons_decl(gen);
#endif

struct	cpu_dep ka6400_calls = {
	ka6400_steal_pages,
	ka6400_mchk,
	ka6400_memerr,
	ka6400_conf,
	generic_gettime,
	generic_settime,
	6,	/* ~VUPS */
	16,	/* SCB pages */
	0,
	0,
	0,
	0,
	0,
#if defined(MULTIPROCESSOR)
	ka6400_startslave,
#endif
};

struct ka6400_softc {
	struct device sc_dev;
	struct cpu_info *sc_ci;
	int sc_nodeid;		/* CPU node ID */
};

CFATTACH_DECL(cpu_xmi, sizeof(struct ka6400_softc),
    ka6400_match, ka6400_attach, NULL, NULL);

static int
ka6400_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct xmi_attach_args *xa = aux;

	if (bus_space_read_2(xa->xa_iot, xa->xa_ioh, XMI_TYPE) != XMIDT_KA64)
		return 0;

	if (cf->cf_loc[XMICF_NODE] != XMICF_NODE_DEFAULT &&
	    cf->cf_loc[XMICF_NODE] != xa->xa_nodenr)
		return 0;

	return 1;
}

static void
ka6400_attach(struct device *parent, struct device *self, void *aux)
{
	struct ka6400_softc *sc = (void *)self;
	struct xmi_attach_args *xa = aux;
	int vp;

	vp = (cca->cca_vecenab & (1 << xa->xa_nodenr));
	printf("\n%s: ka6400 (%s) rev %d%s\n", self->dv_xname,
	    mastercpu == xa->xa_nodenr ? "master" : "slave",
	    bus_space_read_4(xa->xa_iot, xa->xa_ioh, XMI_TYPE) >> 16,
	    (vp ? ", vector processor present" : ""));

	sc->sc_nodeid = xa->xa_nodenr;

	if (xa->xa_nodenr != mastercpu) {
#if defined(MULTIPROCESSOR)
		sc->sc_ci = cpu_slavesetup(self);
		v_putc = ka6400_putc;	/* Need special console handling */
#endif
		return;
	}

	mtpr(0, PR_VPSR); /* Can't use vector processor */
	curcpu()->ci_dev = self;
}

void
ka6400_conf()
{
	int mapaddr;

	rssc = (void *)vax_map_physmem(RSSC_ADDR, 1);
	mastercpu = rssc[RSSC_IPORT/4] & 15;
	mapaddr = (cca ? (int)cca : rpb.cca_addr);
	cca = (void *)vax_map_physmem(mapaddr, vax_btoc(sizeof(struct cca)));
}

/*
 * MS62 support.
 * This code should:
 *	1: Be completed.
 *	2: (eventually) move to dev/xmi/; it is used by Mips also.
 */
#define MEMRD(reg) bus_space_read_4(sc->sc_iot, sc->sc_ioh, (reg))
#define MEMWR(reg, val) bus_space_write_4(sc->sc_iot, sc->sc_ioh, (reg), (val))

#define MS62_TYPE	0
#define MS62_XBE	4
#define MS62_SEADR	16
#define	MS62_CTL1	20
#define MS62_ECCERR	24
#define MS62_ECCEA	28
#define MS62_ILK0	32
#define MS62_ILK1	36
#define MS62_ILK2	40
#define MS62_ILK3	44
#define MS62_CTL2	48

static int ms6400_match(struct device *, struct cfdata *, void *);
static void ms6400_attach(struct device *, struct device *, void*);

struct mem_xmi_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

CFATTACH_DECL(mem_xmi, sizeof(struct mem_xmi_softc),
    ms6400_match, ms6400_attach, NULL, NULL);

static int
ms6400_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct xmi_attach_args *xa = aux;

	if (bus_space_read_2(xa->xa_iot, xa->xa_ioh, XMI_TYPE) != XMIDT_MS62)
		return 0;

	if (cf->cf_loc[XMICF_NODE] != XMICF_NODE_DEFAULT &&
	    cf->cf_loc[XMICF_NODE] != xa->xa_nodenr)
		return 0;

	return 1;
}

static void
ms6400_attach(struct device *parent, struct device *self, void *aux)
{
	struct mem_xmi_softc *sc = (void *)self;
	struct xmi_attach_args *xa = aux;

	sc->sc_iot = xa->xa_iot;
	sc->sc_ioh = xa->xa_ioh;
	printf("\n%s: MS62, rev %d, size 32MB\n", self->dv_xname,
	    MEMRD(MS62_TYPE) >> 16);
}

static void
ka6400_memerr()
{
	printf("ka6400_memerr\n");
}

struct mc6400frame {
	int	mc64_summary;		/* summary parameter */
	int	mc64_va;		/* va register */
	int	mc64_vb;		/* memory address */
	int	mc64_sisr;		/* status word */
	int	mc64_state;		/* error pc */
	int	mc64_sc;		/* micro pc */
	int	mc64_pc;		/* current pc */
	int	mc64_psl;		/* current psl */
};

static int
ka6400_mchk(void *cmcf)
{
	return (MCHK_PANIC);
}

#if defined(MULTIPROCESSOR)
#define RXBUF	80
static char rxbuf[RXBUF];
static int got = 0, taken = 0;
static int expect = 0;
#endif
#if 0
/*
 * Receive a character from logical console.
 */
static void
rxcdintr(void *arg)
{
	int c = mfpr(PR_RXCD);

	if (c == 0)
		return;

#if defined(MULTIPROCESSOR)
	if ((c & 0xff) == 0) {
		if (curcpu()->ci_flags & CI_MASTERCPU)
			ka6400_cnintr();
		return;
	}

	if (expect == ((c >> 8) & 0xf))
		rxbuf[got++] = c & 0xff;

	if (got == RXBUF)
		got = 0;
#endif
}
#endif

/*
 * From ka670, which has the same cache structure.
 */
static void
ka6400_enable_cache(void)
{
	mtpr(KA670_PCS_REFRESH, PR_PCSTS);	/* disable primary cache */
	mtpr(mfpr(PR_PCSTS), PR_PCSTS);		/* clear error flags */
	mtpr(8, PR_BCCTL);			/* disable backup cache */
	mtpr(0, PR_BCFBTS);	/* flush backup cache tag store */
	mtpr(0, PR_BCFPTS);	/* flush primary cache tag store */
	mtpr(0x0e, PR_BCCTL);	/* enable backup cache */
	mtpr(KA670_PCS_FLUSH | KA670_PCS_REFRESH, PR_PCSTS);	/* flush primary cache */
	mtpr(KA670_PCS_ENABLE | KA670_PCS_REFRESH, PR_PCSTS);	/* flush primary cache */
}

void
ka6400_steal_pages(void)
{
	int i, ncpus;

	ka6400_enable_cache(); /* Turn on cache early */
	if (cca == 0)
		cca = (void *)rpb.cca_addr;
	/* Is there any way to get number of CPUs easier??? */
	for (i = ncpus = 0; i < cca->cca_maxcpu; i++)
		if (cca->cca_console & (1 << i))
			ncpus++;
	sprintf(cpu_model, "VAX 6000/4%x0", ncpus + 1);
}
	

#if defined(MULTIPROCESSOR) && 0
int
rxchar()
{
	int ret;

	if (got == taken)
		return 0;

	ret = rxbuf[taken++];
	if (taken == RXBUF)
		taken = 0;
	return ret;
}

static void
ka6400_startslave(struct device *dev, struct cpu_info *ci)
{
	struct ka6400_softc *sc = (void *)dev;
	int id = sc->sc_binid;
	int i;

	expect = sc->sc_binid;
	/* First empty queue */
	for (i = 0; i < 10000; i++)
		if (rxchar())
			i = 0;
	ka6400_txrx(id, "\020", 0);		/* Send ^P to get attention */
	ka6400_txrx(id, "I\r", 0);			/* Init other end */
	ka6400_txrx(id, "D/I 4 %x\r", ci->ci_istack);	/* Interrupt stack */
	ka6400_txrx(id, "D/I C %x\r", mfpr(PR_SBR));	/* SBR */
	ka6400_txrx(id, "D/I D %x\r", mfpr(PR_SLR));	/* SLR */
	ka6400_txrx(id, "D/I 10 %x\r", (int)ci->ci_pcb);	/* PCB for idle proc */
	ka6400_txrx(id, "D/I 11 %x\r", mfpr(PR_SCBB));	/* SCB */
	ka6400_txrx(id, "D/I 38 %x\r", mfpr(PR_MAPEN)); /* Enable MM */
	ka6400_txrx(id, "S %x\r", (int)&vax_mp_tramp); /* Start! */
	expect = 0;
	for (i = 0; i < 10000; i++)
		if ((volatile)ci->ci_flags & CI_RUNNING)
			break;
	if (i == 10000)
		printf("%s: (ID %d) failed starting??!!??\n",
		    dev->dv_xname, sc->sc_binid);
}

void
ka6400_txrx(int id, const char *fmt, int arg)
{
	char buf[20];

	sprintf(buf, fmt, arg);
	ka6400_sendstr(id, buf);
	ka6400_sergeant(id);
}

void
ka6400_sendstr(int id, const char *buf)
{
	register u_int utchr; /* Ends up in R11 with PCC */
	int ch, i;

	while (*buf) {
		utchr = *buf | id << 8;

		/*
		 * It seems like mtpr to TXCD sets the V flag if it fails.
		 * Cannot check that flag in C...
		 */
#ifdef __GNUC__
		__asm("1:;mtpr %0,$92;bvs 1b" :: "g"(utchr));
#else
		__asm("1:;mtpr r11,$92;bvs 1b");
#endif
		buf++;
		i = 30000;
		while ((ch = rxchar()) == 0 && --i)
			;
		if (ch == 0)
			continue; /* failed */
	}
}

void
ka6400_sergeant(int id)
{
	int i, ch, nserg;

	nserg = 0;
	for (i = 0; i < 30000; i++) {
		if ((ch = rxchar()) == 0)
			continue;
		if (ch == '>')
			nserg++;
		else
			nserg = 0;
		i = 0;
		if (nserg == 3)
			break;
	}
	/* What to do now??? */
}

/*
 * Write to master console.
 * Need no locking here; done in the print functions.
 */
static volatile int ch = 0;

void
ka6400_putc(int c)
{
	if (curcpu()->ci_flags & CI_MASTERCPU) {
		gencnputc(0, c);
		return;
	}
	ch = c;
	mtpr(mastercpu << 8, PR_RXCD); /* Send IPI to mastercpu */
	while (ch != 0)
		; /* Wait for master to handle */
}

/*
 * Got character IPI.
 */
void
ka6400_cnintr()
{
	if (ch != 0)
		gencnputc(0, ch);
	ch = 0; /* Release slavecpu */
}
#endif
