/*	$NetBSD: uba_sbi.c,v 1.26.2.1 2009/05/13 17:18:41 jym Exp $	   */
/*
 * Copyright (c) 1982, 1986 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)uba.c	7.10 (Berkeley) 12/16/90
 *	@(#)autoconf.c	7.20 (Berkeley) 5/9/91
 */

/*
 * Copyright (c) 1996 Jonathan Stone.
 * Copyright (c) 1994, 1996 Ludd, University of Lule}, Sweden.
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
 *	@(#)uba.c	7.10 (Berkeley) 12/16/90
 *	@(#)autoconf.c	7.20 (Berkeley) 5/9/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uba_sbi.c,v 1.26.2.1 2009/05/13 17:18:41 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#define _VAX_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/mtpr.h>
#include <machine/nexus.h>
#include <machine/cpu.h>
#include <machine/sgmap.h>
#include <machine/scb.h>

#include <dev/qbus/ubavar.h>

#include <vax/uba/uba_common.h>

#include "locators.h"
#include "ioconf.h"

/* Some SBI-specific defines */
#define UBASIZE		(UBAPAGES * VAX_NBPG)
#define UMEMA8600(i)   	(0x20100000+(i)*0x40000)
#define	UMEMB8600(i)	(0x22100000+(i)*0x40000)

/*
 * Some status registers.
 */
#define UBACNFGR_UBIC  0x00010000      /* unibus init complete */
#define UBACNFGR_BITS \
"\40\40PARFLT\37WSQFLT\36URDFLT\35ISQFLT\34MXTFLT\33XMTFLT\30ADPDN\27ADPUP\23UBINIT\22UBPDN\21UBIC"

#define UBACR_IFS      0x00000040      /* interrupt field switch */
#define UBACR_BRIE     0x00000020      /* BR interrupt enable */
#define UBACR_USEFIE   0x00000010      /* UNIBUS to SBI error field IE */
#define UBACR_SUEFIE   0x00000008      /* SBI to UNIBUS error field IE */
#define UBACR_ADINIT   0x00000001      /* adapter init */

#define UBADPR_BNE     0x80000000      /* buffer not empty - purge */

#define UBABRRVR_DIV    0x0000ffff      /* device interrupt vector field */

#define UBASR_BITS \
"\20\13RDTO\12RDS\11CRD\10CXTER\7CXTMO\6DPPE\5IVMR\4MRPF\3LEB\2UBSTO\1UBSSYNTO"

const char ubasr_bits[] = UBASR_BITS;

/*
 * The DW780 are directly connected to the SBI on 11/780 and 8600.
 */
static	int	dw780_match(device_t, cfdata_t, void *);
static	void	dw780_attach(device_t, device_t, void *);
static	void	dw780_init(struct uba_softc*);
static	void    dw780_beforescan(struct uba_softc *);
static	void    dw780_afterscan(struct uba_softc *);
static	int     dw780_errchk(struct uba_softc *);
static	inline	void uba_dw780int_common(void *, int);
static	void    uba_dw780int_0x14(void *);
static	void    uba_dw780int_0x15(void *);
static	void    uba_dw780int_0x16(void *);
static	void    uba_dw780int_0x17(void *);
static  void	ubaerror(struct uba_softc *, int *, int *);
#ifdef notyet
static	void	dw780_purge(struct uba_softc *, int);
#endif

CFATTACH_DECL_NEW(uba_sbi, sizeof(struct uba_vsoftc),
    dw780_match, dw780_attach, NULL, NULL);

static struct evcnt strayint = EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "uba","stray intr");
static int strayinit = 0;

extern	struct vax_bus_space vax_mem_bus_space;

int
dw780_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sbi_attach_args * const sa = aux;

	if (cf->cf_loc[SBICF_TR] != sa->sa_nexnum &&
	    cf->cf_loc[SBICF_TR] != SBICF_TR_DEFAULT)
		return 0;
	/*
	 * The uba type is actually only telling where the uba
	 * space is in nexus space.
	 */
	if ((sa->sa_type & ~3) != NEX_UBA0)
		return 0;

	return 1;
}

void
dw780_attach(device_t parent, device_t self, void *aux)
{
	struct uba_vsoftc * const sc = device_private(self);
	struct sbi_attach_args * const sa = aux;
	int ubaddr = sa->sa_type & 3;

	aprint_normal(": DW780\n");

	/*
	 * Fill in bus specific data.
	 */
	sc->uv_sc.uh_dev = self;
	sc->uv_sc.uh_ubainit = dw780_init;
#ifdef notyet
	sc->uv_sc.uh_ubapurge = dw780_purge;
#endif
	sc->uv_sc.uh_beforescan = dw780_beforescan;
	sc->uv_sc.uh_afterscan = dw780_afterscan;
	sc->uv_sc.uh_errchk = dw780_errchk;
	sc->uv_sc.uh_iot = sa->sa_iot;
	sc->uv_sc.uh_dmat = &sc->uv_dmat;
	sc->uv_sc.uh_nr = ubaddr;
	sc->uv_uba = (void *)sa->sa_ioh;
	sc->uh_ibase = VAX_NBPG + ubaddr * VAX_NBPG;
	sc->uv_sc.uh_type = UBA_UBA;

	if (strayinit++ == 0) evcnt_attach_static(&strayint); /* Setup stray
							         interrupt
							         counter. */

	/*
	 * Set up dispatch vectors for DW780.
	 */
#define SCB_VECALLOC_DW780(br)						\
	scb_vecalloc(vecnum(0, br, sa->sa_nexnum), uba_dw780int_ ## br,	\
		     sc, SCB_ISTACK, &sc->uv_sc.uh_intrcnt)		\

	SCB_VECALLOC_DW780(0x14);
	SCB_VECALLOC_DW780(0x15);
	SCB_VECALLOC_DW780(0x16);
	SCB_VECALLOC_DW780(0x17);

	evcnt_attach_dynamic(&sc->uv_sc.uh_intrcnt, EVCNT_TYPE_INTR, NULL,
		device_xname(sc->uv_sc.uh_dev), "intr");

	/*
	 * Fill in variables used by the sgmap system.
	 */
	sc->uv_size = UBASIZE;		/* Size in bytes of Unibus space */

	uba_dma_init(sc);
	uba_attach(&sc->uv_sc, (sa->sa_sbinum ? UMEMB8600(ubaddr) :
	    UMEMA8600(ubaddr)) + (UBAPAGES * VAX_NBPG));
}

void
dw780_beforescan(struct uba_softc *sc)
{
	struct uba_vsoftc * const vc = (void *)sc;
	volatile int *hej = &vc->uv_uba->uba_sr;

	*hej = *hej;
	vc->uv_uba->uba_cr = UBACR_IFS|UBACR_BRIE;
}

void
dw780_afterscan(struct uba_softc *sc)
{
	struct uba_vsoftc * const vc = (void *)sc;

	vc->uv_uba->uba_cr = UBACR_IFS | UBACR_BRIE |
	    UBACR_USEFIE | UBACR_SUEFIE |
	    (vc->uv_uba->uba_cr & 0x7c000000);
}


int
dw780_errchk(struct uba_softc *sc)
{
	struct uba_vsoftc * const vc = (void *)sc;
	volatile int *hej = &vc->uv_uba->uba_sr;

	if (*hej) {
		*hej = *hej;
		return 1;
	}
	return 0;
}

static inline void
uba_dw780int_common(void *arg, int br)
{
	extern	void scb_stray(void *);
	struct	uba_vsoftc *vc = arg;
	struct	uba_regs *ur = vc->uv_uba;
	struct	ivec_dsp *ivec;
	struct  evcnt *uvec;
	int	vec;

	uvec = &vc->uv_sc.uh_intrcnt;
	vec = ur->uba_brrvr[br - 0x14];
	if (vec <= 0) {
		ubaerror(&vc->uv_sc, &br, &vec);
		if (vec == 0)
			return;
	}

	uvec->ev_count--;	/* This interrupt should not be counted against
				   the uba. */
	ivec = &scb_vec[(vc->uh_ibase + vec)/4];
	if (cold && *ivec->hoppaddr == scb_stray) {
		scb_fake(vec + vc->uh_ibase, br);
	} else {
		if (*ivec->hoppaddr == scb_stray)
			strayint.ev_count++; /* Count against stray int */
		else
			ivec->ev->ev_count++; /* Count against device */
		(*ivec->hoppaddr)(ivec->pushlarg);
	}
}

#define UBA_DW780INT(br)		\
void					\
uba_dw780int_ ## br(void *arg)		\
{					\
	uba_dw780int_common(arg, br);	\
}					\

UBA_DW780INT(0x14);
UBA_DW780INT(0x15);
UBA_DW780INT(0x16);
UBA_DW780INT(0x17);

void
dw780_init(struct uba_softc *sc)
{
	struct uba_vsoftc *vc = (void *)sc;

	vc->uv_uba->uba_cr = UBACR_ADINIT;
	vc->uv_uba->uba_cr = UBACR_IFS|UBACR_BRIE|UBACR_USEFIE|UBACR_SUEFIE;
	while ((vc->uv_uba->uba_cnfgr & UBACNFGR_UBIC) == 0)
		;
}

#ifdef notyet
void
dw780_purge(struct uba_softc *sc, int bdp)
{
	struct uba_vsoftc *vc = (void *)sc;

	vc->uv_uba->uba_dpr[bdp] |= UBADPR_BNE;
}
#endif

int	ubawedgecnt = 10;
int	ubacrazy = 500;
int	zvcnt_max = 5000;	/* in 8 sec */
int	ubaerrcnt;
/*
 * This routine is called by the locore code to process a UBA
 * error on an 11/780 or 8600.	The arguments are passed
 * on the stack, and value-result (through some trickery).
 * In particular, the uvec argument is used for further
 * uba processing so the result aspect of it is very important.
 */
/*ARGSUSED*/
void
ubaerror(struct uba_softc *uh, int *ipl, int *uvec)
{
	struct uba_vsoftc *vc = (void *)uh;
	struct	uba_regs *uba = vc->uv_uba;
	int sr, s;
	char sbuf[256], sbuf2[256];

	if (*uvec == 0) {
		/*
		 * Declare dt as unsigned so that negative values
		 * are handled as >8 below, in case time was set back.
		 */
		u_long	dt = time_second - vc->uh_zvtime;

		vc->uh_zvtotal++;
		if (dt > 8) {
			vc->uh_zvtime = time_second;
			vc->uh_zvcnt = 0;
		}
		if (++vc->uh_zvcnt > zvcnt_max) {
			aprint_error_dev(vc->uv_sc.uh_dev,
			    "too many zero vectors (%d in <%d sec)\n",
			    vc->uh_zvcnt, (int)dt + 1);

			snprintb(sbuf, sizeof(sbuf), UBACNFGR_BITS,
			    uba->uba_cnfgr & ~0xff);
			aprint_error(
			    "\tIPL 0x%x\n"
			    "\tcnfgr: %s\tAdapter Code: 0x%x\n",
			    *ipl, sbuf, uba->uba_cnfgr&0xff);

			snprintb(sbuf, sizeof(sbuf), ubasr_bits, uba->uba_sr);
			aprint_error(
			    "\tsr: %s\n"
			    "\tdcr: %x (MIC %sOK)\n",
			    sbuf, uba->uba_dcr,
			    (uba->uba_dcr&0x8000000)?"":"NOT ");

			ubareset(&vc->uv_sc);
		}
		return;
	}
	if (uba->uba_cnfgr & NEX_CFGFLT) {
		snprintb(sbuf, sizeof(sbuf), ubasr_bits, uba->uba_sr);
		snprintb(sbuf2, sizeof(sbuf2), NEXFLT_BITS, uba->uba_cnfgr);
		aprint_error_dev(vc->uv_sc.uh_dev,
		    "sbi fault sr=%s cnfgr=%s\n", sbuf, sbuf2);
		ubareset(&vc->uv_sc);
		*uvec = 0;
		return;
	}
	sr = uba->uba_sr;
	s = spluba();
	snprintb(sbuf, sizeof(sbuf), ubasr_bits, uba->uba_sr);
	aprint_error_dev(vc->uv_sc.uh_dev,
	    "uba error sr=%s fmer=%x fubar=%o\n",
	    sbuf, uba->uba_fmer, 4*uba->uba_fubar);
	splx(s);
	uba->uba_sr = sr;
	*uvec &= UBABRRVR_DIV;
	if (++ubaerrcnt % ubawedgecnt == 0) {
		if (ubaerrcnt > ubacrazy)
			panic("uba crazy");
		printf("ERROR LIMIT ");
		ubareset(&vc->uv_sc);
		*uvec = 0;
	}
}
