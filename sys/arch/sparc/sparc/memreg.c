/*	$NetBSD: memreg.c,v 1.23 1998/03/21 20:34:59 pk Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *	This product includes software developed by Harvard University.
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
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Harvard University.
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
 *	@(#)memreg.c	8.1 (Berkeley) 6/11/93
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/ctlreg.h>

#include <sparc/sparc/memreg.h>
#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/cpuvar.h>

#include <machine/reg.h>	/* for trapframe */
#include <machine/trap.h>	/* for trap types */

static int	memregmatch_mainbus
			__P((struct device *, struct cfdata *, void *));
static int	memregmatch_obio
			__P((struct device *, struct cfdata *, void *));
static void	memregattach_mainbus
			__P((struct device *, struct device *, void *));
static void	memregattach_obio
			__P((struct device *, struct device *, void *));

struct cfattach memreg_mainbus_ca = {
	sizeof(struct device), memregmatch_mainbus, memregattach_mainbus
};

struct cfattach memreg_obio_ca = {
	sizeof(struct device), memregmatch_obio, memregattach_obio
};

void memerr __P((int, u_int, u_int, u_int, u_int));
#if defined(SUN4M)
static void hardmemerr4m __P((int, u_int, u_int));
#endif

/*
 * The OPENPROM calls this "memory-error".
 */
static int
memregmatch_mainbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp("memory-error", ma->ma_name) == 0);
}

static int
memregmatch_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;

	if (uoba->uoba_isobio4 == 0)
		return (strcmp("memory-error", uoba->uoba_sbus.sa_name) == 0);

	if (!CPU_ISSUN4) {
		printf("memregmatch_obio: attach args mixed up\n");
		return (0);
	}

	return (1);
}

/* ARGSUSED */
static void
memregattach_mainbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t bh;

	printf("\n");
	if (ma->ma_promvaddr != 0) {
		par_err_reg = (volatile int *)ma->ma_promvaddr;
		return;
	}

	if (sparc_bus_map(ma->ma_bustag, ma->ma_iospace,
			 (bus_addr_t)ma->ma_paddr,
			 sizeof(par_err_reg),
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf("memregattach_mainbus: can't map register\n");
		return;
	}
}

/* ARGSUSED */
static void
memregattach_obio(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	bus_space_handle_t bh;

	if (uoba->uoba_isobio4 == 0) {
		struct sbus_attach_args *sa = &uoba->uoba_sbus;
		if (sa->sa_promvaddr != 0) {
			par_err_reg = (volatile int *)sa->sa_promvaddr;
			return;
		}

		if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
				 sa->sa_offset,
				 sizeof(par_err_reg),
				 BUS_SPACE_MAP_LINEAR,
				 0, &bh) != 0) {
			printf("memregattach_obio: can't map register\n");
			return;
		}
		par_err_reg = (volatile int *)bh;
	}

	/* On sun4, `par_err_reg' has already been mapped in autoconf.c */
	if (par_err_reg == NULL)
		panic("memregattach");

	printf("\n");
}

/*
 * Synchronous and asynchronous memory error handler.
 * (This is the level 15 interrupt, which is not vectored.)
 * Should kill the process that got its bits clobbered,
 * and take the page out of the page pool, but for now...
 */

void
memerr(issync, ser, sva, aer, ava)
	int issync;
	u_int ser, sva, aer, ava;
{
#if defined(SUN4) || defined(SUN4C)
	char bits[64];
#endif

	/* XXX Ugh! Clean up this switch and all the ifdefs! */
	switch (cputyp) {
#if defined(SUN4)
	case CPU_SUN4:
		if (par_err_reg) {
			printf("mem err: ser=%s sva=0x%x\n",
			    bitmask_snprintf(ser, SER_BITS, bits,
			    sizeof(bits)), sva);
			printf("parity error register = %s\n",
				bitmask_snprintf(*par_err_reg, PER_BITS,
				bits, sizeof(bits)));
		} else {
			printf("mem err: ser=? sva=?\n");
			printf("parity error register not mapped yet!\n"); /* XXX */
		}
#ifdef DEBUG
		callrom();
#else
		panic("memory error");		/* XXX */
#endif
		break;
#endif /* Sun4 */

#if defined(SUN4C)
	case CPU_SUN4C:
		printf("%ssync mem arr: ser=%s sva=0x%x ",
		    issync ? "" : "a", bitmask_snprintf(ser, SER_BITS,
		    bits, sizeof(bits)), sva);
		printf("aer=%s ava=0x%x\n", bitmask_snprintf(aer & 0xff,
		    AER_BITS, bits, sizeof(bits)), ava);
		if (par_err_reg)
			printf("parity error register = %s\n",
			    bitmask_snprintf(*par_err_reg, PER_BITS,
			    bits, sizeof(bits)));
#ifdef DEBUG
		callrom();
#else
		panic("memory error");		/* XXX */
#endif
		break;
#endif /* Sun4C */

#if defined(SUN4M)
	case CPU_SUN4M:
		hardmemerr4m(2, ser, sva);
		break;
#endif /* Sun4M */
	    default:
		break;
	}
}


#if defined(SUN4M)
/*
 * hardmemerr4m: called upon fatal memory error. Print a message and panic.
 * Note that issync is not really an indicator of whether or not the error
 * was synchronous; if it is set, it means that the fsr/faddr pair correspond
 * to the MMU's fault status register; if clear, they correspond to the
 * HyperSPARC asynchronous error register. If issync==2, then both decodings
 * of the error register are printed.
 */

static void
hardmemerr4m(issync, fsr, faddr)
	int issync;
	u_int fsr, faddr;
{
	char bits[64];

	switch (issync) {
	    case 1:
		if ((fsr & SFSR_FT) == SFSR_FT_NONE)
		    return;
		printf("mem err: sfsr=%s sfaddr=0x%x\n", bitmask_snprintf(fsr,
		    SFSR_BITS, bits, sizeof(bits)), faddr);
		break;
	    case 0:
		if (!(fsr & AFSR_AFO))
		    return;
		printf("async (HS) mem err: afsr=%s afaddr=0x%x physaddr=0x%x%x\n",
		       bitmask_snprintf(fsr, AFSR_BITS, bits, sizeof(bits)),
		       faddr, (fsr & AFSR_AFA) >> AFSR_AFA_RSHIFT, faddr);
		break;
	    default:	/* unknown; print both decodings*/
		printf("unknown mem err: if sync, fsr=%s fva=0x%x; ",
		    bitmask_snprintf(fsr, SFSR_BITS, bits, sizeof(bits)),
		    faddr);
		printf("if async, fsr=%s fa=0x%x pa=0x%x%x", bitmask_snprintf(fsr,
		    AFSR_BITS, bits, sizeof(bits)), faddr,
		    (fsr & AFSR_AFA) >> AFSR_AFA_RSHIFT, faddr);
		break;
	}
	panic("hard memory error");
}

/*
 * Memerr4m: handle a non-trivial memory fault. These include HyperSPARC
 * asynchronous faults, SuperSPARC store-buffer copyback failures, and
 * data faults without a valid faulting VA. We try to retry the operation
 * once, and then fail if we get called again.
 */

static int addrold = (int) 0xdeadbeef; /* We pick an unlikely address */
static int addroldtop = (int) 0xdeadbeef;
static int oldtype = -1;

void
memerr4m(type, sfsr, sfva, afsr, afva, tf)
	register unsigned type;
	register u_int sfsr;
	register u_int sfva;
	register u_int afsr;
	register u_int afva;
	register struct trapframe *tf;
{
	char bits[64];

	if ((afsr & AFSR_AFO) != 0) {	/* HS async fault! */

		printf("HyperSPARC async cache memory failure at phys 0x%x%x. "
		       "Ignoring.\n", (afsr & AFSR_AFA) >> AFSR_AFA_RSHIFT,
		       afva);

		if (afva == addrold && (afsr & AFSR_AFA) == addroldtop)
			hardmemerr4m(0, afsr, afva);
			/* NOTREACHED */

		oldtype = -1;
		addrold = afva;
		addroldtop = afsr & AFSR_AFA;

	} else if (type == T_STOREBUFFAULT && cpuinfo.cpu_vers == 4) {

		/*
		 * On Supersparc, we try to reenable the store buffers
		 * to force a retry.
		 */
		printf("store buffer copy-back failure at 0x%x. Retrying...\n",
		       sfva);

		if (oldtype == T_STOREBUFFAULT || addrold == sfva)
			hardmemerr4m(1, sfsr, sfva);
			/* NOTREACHED */

		oldtype = T_STOREBUFFAULT;
		addrold = sfva;

		/* reenable store buffer */
		sta(SRMMU_PCR, ASI_SRMMU,
		    lda(SRMMU_PCR, ASI_SRMMU) | VIKING_PCR_SB);

	} else if (type == T_DATAFAULT && !(sfsr & SFSR_FAV)) { /* bizarre */
		/* XXX: Should handle better. See SuperSPARC manual pg. 9-35 */

		printf("warning: got data fault with no faulting address."
		       " Ignoring.\n");

		if (oldtype == T_DATAFAULT)
			hardmemerr4m(1, sfsr, sfva);
			/* NOTREACHED */

		oldtype = T_DATAFAULT;
	} else if (type == 0) {	/* NMI */
		printf("ERROR: got NMI with sfsr=0x%s, sfva=0x%x, ",
		    bitmask_snprintf(sfsr, SFSR_BITS, bits, sizeof(bits)),
		    sfva);
		printf("afsr=0x%s, afaddr=0x%x. Retrying...\n",
		    bitmask_snprintf(afsr, AFSR_BITS, bits, sizeof(bits)),
		    afva);
		if (oldtype == 0 || addrold == sfva)
			hardmemerr4m(1, sfsr, sfva);	/* XXX: async? */
			/* NOTREACHED */

		oldtype = 0;
		addrold = sfva;
	} else 	/* something we don't know about?!? */ {
		printf("unknown fatal memory error, type=%d, sfsr=%s, sfva=0x%x",
		    type, bitmask_snprintf(sfsr, SFSR_BITS, bits, sizeof(bits)),
		    sfva);
		printf(", afsr=%s, afaddr=0x%x\n", bitmask_snprintf(afsr,
		    AFSR_BITS, bits, sizeof(bits)), afva);
		panic("memerr4m");
	}

	return;
}
#endif /* 4m */
