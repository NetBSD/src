/*	$NetBSD: cpu.c,v 1.13.2.1 2004/08/03 10:40:08 skrll Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
 * Copyright (c) 2001 Jason R. Thorpe.
 * Copyright (c) 2004 Christopher SEKIYA
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.13.2.1 2004/08/03 10:40:08 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

static int	cpu_match(struct device *, struct cfdata *, void *);
static void	cpu_attach(struct device *, struct device *, void *);
void		cpu_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
void *cpu_intr_establish(int, int, int (*func)(void *), void *);

static struct evcnt mips_int0_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 0");

static struct evcnt mips_int1_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 1");

static struct evcnt mips_int2_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 2");

static struct evcnt mips_int3_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 3");

static struct evcnt mips_int4_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 4");

static struct evcnt mips_int5_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 5");

static struct evcnt mips_spurint_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "spurious interrupts");

CFATTACH_DECL(cpu, sizeof(struct device),
    cpu_match, cpu_attach, NULL, NULL);

static int
cpu_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

static void
cpu_attach(struct device *parent, struct device *self, void *aux)
{
	printf(": ");
	cpu_identify();
}

/*
 * NB: Do not re-enable interrupts here -- reentrancy here can cause all
 * sorts of Bad Things(tm) to happen, including kernel stack overflows.
 */
void
cpu_intr(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{
	uvmexp.intrs++;

	(void)(*platform.watchdog_reset)();

	if (ipending & MIPS_HARD_INT_MASK) {
        	if (ipending & MIPS_INT_MASK_5) {
               		(void)(*platform.intr5)(status, cause, pc, ipending);
			mips_int5_evcnt.ev_count++;
			cause &= ~MIPS_INT_MASK_5;
        	}

		if (ipending & MIPS_INT_MASK_4) {
			(void)(*platform.intr4)(status, cause, pc, ipending);
			mips_int4_evcnt.ev_count++;
			cause &= ~MIPS_INT_MASK_4;
		}

		if (ipending & MIPS_INT_MASK_3) {
			(void)(*platform.intr3)(status, cause, pc, ipending);
			mips_int3_evcnt.ev_count++;
			cause &= ~MIPS_INT_MASK_3;
		}

	        if (ipending & MIPS_INT_MASK_2) {
			(void)(*platform.intr2)(status, cause, pc, ipending);
			mips_int2_evcnt.ev_count++;
			cause &= ~MIPS_INT_MASK_2;
		}

		if (ipending & MIPS_INT_MASK_1) {
			(void)(*platform.intr1)(status, cause, pc, ipending);
			mips_int1_evcnt.ev_count++;
			cause &= ~MIPS_INT_MASK_1;
		}

		if (ipending & MIPS_INT_MASK_0) {  
			(void)(*platform.intr0)(status, cause, pc, ipending);
			mips_int0_evcnt.ev_count++;
			cause &= ~MIPS_INT_MASK_0;
		}

		if (cause & status & MIPS_HARD_INT_MASK)
			mips_spurint_evcnt.ev_count++;
	}

	/* software interrupt */
	ipending &= (MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;

	_clrsoftintr(ipending);

	softintr_dispatch(ipending);
}

void *
cpu_intr_establish(int level, int ipl, int (*func)(void *), void *arg)
{
	(*platform.intr_establish)(level, ipl, func, arg);
	return (void *) -1;
}
