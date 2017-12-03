/*	$NetBSD: ka730.c,v 1.3.12.3 2017/12/03 11:36:48 jdolecek Exp $ */
/*
 * Copyright (c) 1982, 1986, 1988 The Regents of the University of California.
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
 *	@(#)ka730.c	7.4 (Berkeley) 5/9/91
 *	@(#)autoconf.c	7.20 (Berkeley) 5/9/91
 */

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *	@(#)ka730.c	7.4 (Berkeley) 5/9/91
 *	@(#)autoconf.c	7.20 (Berkeley) 5/9/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ka730.c,v 1.3.12.3 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <machine/ka730.h>
#include <machine/clock.h>
#include <machine/sid.h>

#include <vax/vax/gencons.h>

#include "locators.h"

void	ctuattach(void);
static	void ka730_clrf(void);
static	void ka730_conf(void);
static	void ka730_memerr(void);
static	int ka730_mchk(void *t);
static	void ka730_attach_cpu(device_t);

static const char * const ka730_devs[] = { "cpu", "ubi", NULL };

struct	cpu_dep ka730_calls = {
	.cpu_mchk	= ka730_mchk,
	.cpu_memerr	= ka730_memerr,
	.cpu_conf	= ka730_conf,
	.cpu_gettime	= generic_gettime,
	.cpu_settime	= generic_settime,
	.cpu_vups	= 1,	/* ~VUPS */
	.cpu_scbsz	= 4,	/* SCB pages */
	.cpu_clrf	= ka730_clrf,
	.cpu_devs	= ka730_devs,
	.cpu_attach_cpu	= ka730_attach_cpu,

};

void
ka730_conf(void)
{
	/* Call ctuattach() here so it can setup its vectors. */
	ctuattach();
}

void
ka730_attach_cpu(device_t self)
{
	aprint_normal(": KA730, ucode rev %d\n", V730UCODE(vax_cpudata));
}

static void ka730_memenable(device_t, device_t, void *);
static int ka730_memmatch(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(mem_ubi, 0,
    ka730_memmatch, ka730_memenable, NULL, NULL);

int
ka730_memmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct	sbi_attach_args *sa = aux;

	if (cf->cf_loc[UBICF_TR] != sa->sa_nexnum &&
	    cf->cf_loc[UBICF_TR] > UBICF_TR_DEFAULT)
		return 0;

	if (sa->sa_type != NEX_MEM16)
		return 0;

	return 1;
}

struct	mcr730 {
	int	mc_addr;		/* address and syndrome */
	int	mc_err;			/* error bits */
};

/* enable crd interrupts */
void
ka730_memenable(device_t parent, device_t self, void *aux)
{
}

/* log crd errors */
void
ka730_memerr(void)
{
}

#define NMC730 12
const char *mc730[]={"0","1","2","3","4","5","6","7","8","9","10","11","12","13",
	"14","15"};

struct mc730frame {
	int	mc3_bcnt;		/* byte count == 0xc */
	int	mc3_summary;		/* summary parameter (as above) */
	int	mc3_parm[2];		/* parameters */
	int	mc3_pc;			/* trapped pc */
	int	mc3_psl;		/* trapped psl */
};

int
ka730_mchk(void *cmcf)
{
	register struct mc730frame *mcf = (struct mc730frame *)cmcf;
	register int type = mcf->mc3_summary;

	printf("machine check %x: %s\n",
	       type, type < NMC730 ? mc730[type] : "???");
	printf("\tparams %x %x pc %x psl %x mcesr %x\n",
	       mcf->mc3_parm[0], mcf->mc3_parm[1],
	       mcf->mc3_pc, mcf->mc3_psl, mfpr(PR_MCESR));
	mtpr(0xf, PR_MCESR);
	return (MCHK_PANIC);
}

void
ka730_clrf(void)
{
	int s = splhigh();

#define WAIT	while ((mfpr(PR_TXCS) & GC_RDY) == 0) ;

	WAIT;

	mtpr(GC_CWFL|GC_CONS, PR_TXDB);

	WAIT;
	mtpr(GC_CCFL|GC_CONS, PR_TXDB);

	WAIT;
	splx(s);
}
