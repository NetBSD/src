/*	$NetBSD: vme_two.c,v 1.1 1999/02/20 00:12:01 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * VME support specific to the VMEchip2 found on the MVME-1[67]7 boards.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/psl.h>
#include <machine/cpu.h>

#include <mvme68k/dev/vmevar.h>
#include <mvme68k/dev/vme_twovar.h>
#include <mvme68k/dev/vme_tworeg.h>

int 	vmetwo_match  __P((struct device *, struct cfdata *, void *));
void	vmetwo_attach __P((struct device *, struct device *, void *));

struct cfattach vmetwo_ca = {
	sizeof(struct vmechip_softc), vmetwo_match, vmetwo_attach
};

extern struct cfdriver vmechip_cd;
extern struct cfdriver vmes_cd;
extern struct cfdriver vmel_cd;

int	vmetwo_translate_addr __P((u_long, size_t, int, int, u_long *));
void	vmetwo_intrline_enable __P((int));
void	vmetwo_intrline_disable __P((int));

struct vme_chip vme_two_switch = {
	vmetwo_translate_addr,
	vmetwo_intrline_enable,
	vmetwo_intrline_disable
};

struct vme_two_lcsr *sys_vme_two;
struct vme_two_gcsr *sys_vme_two_gcsr;


/*
 * Array of interrupt handlers registered with us for the non-VMEbus
 * vectored interrupts. Eg. ABORT Switch, SYSFAIL etc.
 *
 * We can't just install a caller's handler directly, since these
 * interrupts have to be manually cleared, so we have a trampoline
 * which does the clearing automatically.
 */
static struct vme_two_handler {
	void	(*isr_hand) __P((void *));
	void	*isr_arg;
} vme_two_handlers[(VME2_VECTOR_MAX - VME2_VECTOR_MIN) + 1];

#define VMETWO_HANDLERS_SZ	(sizeof(vme_two_handlers) /	\
				 sizeof(struct vme_two_handler))

static int vmetwo_isr_trampoline __P((void *));


int
vmetwo_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	char *ma_name = (char *)aux;

	/* Only one VMEchip2, please. */
	if ( sys_vme_two )
		return 0;

	if ( strcmp(ma_name, vmechip_cd.cd_name) )
		return 0;

	return 1;
}

void
vmetwo_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct vmechip_softc *sc = (struct vmechip_softc *)self;
	struct vme_two_lcsr *vme;
	int i;

	/* Glue into generic VME code. */
	sc->sc_reg = VMECHIP2_VADDR(VME2_REGS_LCSR);
	sc->sc_chip = &vme_two_switch;

	/* Clear out the ISR handler array */
	for (i = 0; i < VMETWO_HANDLERS_SZ; i++) {
		vme_two_handlers[i].isr_hand = NULL;
		vme_two_handlers[i].isr_arg = NULL;
	}

	/* Initialize the chip. */
	vme = (struct vme_two_lcsr *)sc->sc_reg;
	sys_vme_two = vme;
	sys_vme_two_gcsr = VMECHIP2_VADDR(VME2_REGS_GCSR);

	/* Firstly, disable all VMEChip2 Interrupts */
	vme->vme_io_ctrl1 &= ~VME2_IO_CTRL1_MIEN;
	vme->vme_lbint_enable = 0;

	/* Zap all the IRQ level registers */
	for (i = 0; i < VME2_NUM_IL_REGS; i++)
		vme->vme_irq_levels[i] = 0;

	/* Disable the VMEbus Slave Windows */
	vme->vme_slave_ctrl = 0;

	/* Disable the VMEbus Master Windows */
	vme->vme_local_2_vme_enab = 0;
	for (i = 0; i < VME2_MASTER_WINDOWS; i++)
		vme->vme_master_win[i] = 0;

	/* Disable VMEbus I/O access */
	vme->vme_local_2_vme_ctrl &= ~VME2_LOC2VME_MASK;

	/* Disable the tick timers */
	vme->vme_tt1_ctrl &= ~VME2_TICK1_TMR_MASK;
	vme->vme_tt2_ctrl &= ~VME2_TICK2_TMR_MASK;

	/* Set the VMEChip2's vector base register to the required value */
	vme->vme_vector_base &= ~VME2_VECTOR_BASE_MASK;
	vme->vme_vector_base |=  VME2_VECTOR_BASE_REG;

	/* Set the Master Interrupt Enable bit now */
	vme->vme_io_ctrl1 |= VME2_IO_CTRL1_MIEN;

	/* Let the NMI handler deal with level 7 ABORT switch interrupts */
	vmetwo_intr_establish(VME2_VEC_ABORT, (void *)nmihand, 7, NULL);

	printf(": Type 2 VMEchip, scon jumper %s\n",
	    (vme->vme_board_ctrl & VME2_BRD_CTRL_SCON)? "enabled" : "disabled");

	/* Attach children. */
	vme_config(sc);
}

/*
 * Install a handler for the vectored interrupts generated by
 * the VMEchip2 for things like the ABORT switch, VMEbus ACFAIL
 * and SYSFAIL, Memory Parity Error and so on.
 *
 * Note: These are distinct from real VMEbus interrupts where the
 *       vector comes in over the bus in that they have to be
 *       manually cleared.
 */
void
vmetwo_intr_establish(vec, hand, lvl, arg)
	int vec;
	void (*hand) __P((void *));
	int lvl;
	void *arg;
{
	u_int8_t ilval;
	int ilreg;
	int ilshift;

	if ( vec < VME2_VECTOR_MIN || vec > VME2_VECTOR_MAX ) {
		printf("vmetwo: Illegal vector offset: 0x%x\n", vec);
		panic("vmetwo_intr_establish");
	}

	if ( lvl < 1 || lvl > 7 ) {
		printf("vmetwo: Illegal interrupt level: %d\n", lvl);
		panic("vmetwo_intr_establish");
	}

#ifdef DEBUG
	if ( (sys_vme_two->vme_lbint_enable & VME2_LBINT_ENABLE(vec)) != 0 ) {
		printf("vmetwo: Interrupt vector 0x%x already enabled\n", vec);
		panic("vmetwo_intr_establish");
	}
#endif

	ilreg = VME2_ILREG_FROM_VEC(vec);
	ilshift = VME2_ILSHFT_FROM_VEC(vec);

	ilval = (sys_vme_two->vme_irq_levels[ilreg] >> ilshift) & VME2_ILMASK;

#ifdef DEBUG
	if ( ilval != 0 && ilval != lvl ) {
		printf("vmetwo: Vector 0x%x: Level register non-zero (%d)\n",
			vec, ilval);
		panic("vmetwo_intr_establish");
	}

	if ( vme_two_handlers[vec - VME2_VECTOR_MIN].isr_hand != NULL ) {
		printf("vmetwo: Vector 0x%x already claimed\n", vec);
		panic("vmetwo_intr_establish");
	}
#endif

	vme_two_handlers[vec - VME2_VECTOR_MIN].isr_hand = hand;
	vme_two_handlers[vec - VME2_VECTOR_MIN].isr_arg = arg;

	/* Hook the interrupt */
	isrlink_vectored(vmetwo_isr_trampoline, (void *)vec,
			 lvl, vec + VME2_VECTOR_BASE);

	/* Program the specified interrupt to signal at 'lvl' */
	ilval = sys_vme_two->vme_irq_levels[ilreg];
	ilval &= ~(VME2_ILMASK << ilshift);
	sys_vme_two->vme_irq_levels[ilreg] = ilval | (lvl << ilshift);

	/* Clear it */
	sys_vme_two->vme_lbint_clear |= VME2_LBINT_CLEAR(vec);

	/* Enable it. */
	sys_vme_two->vme_lbint_enable |= VME2_LBINT_ENABLE(vec);
}

void
vmetwo_intr_disestablish(vec)
	int vec;
{
	u_int8_t ilval;
	int ilreg;
	int ilshift;

	if ( vec < VME2_VECTOR_MIN || vec > VME2_VECTOR_MAX ) {
		printf("vmetwo: Illegal vector offset: 0x%x\n", vec);
		panic("vmetwo_intr_disestablish");
	}

#ifdef DEBUG
	if ( (sys_vme_two->vme_lbint_enable & VME2_LBINT_ENABLE(vec)) == 0 ) {
		printf("vmetwo: Interrupt vector 0x%x already disabled\n", vec);
		panic("vmetwo_intr_disestablish");
	}
#endif

	ilreg = VME2_ILREG_FROM_VEC(vec);
	ilshift = VME2_ILSHFT_FROM_VEC(vec);
	ilval = sys_vme_two->vme_irq_levels[ilreg];

#ifdef DEBUG
	if ( ((ilval >> ilshift) & VME2_ILMASK) == 0 ) {
		printf("vmetwo: Vector 0x%x: Level register is zero (%d)\n",
			vec, ilval);
		panic("vmetwo_intr_establish");
	}

	if ( vme_two_handlers[vec - VME2_VECTOR_MIN].isr_hand == NULL ) {
		printf("vmetwo: Vector 0x%x already unclaimed\n", vec);
		panic("vmetwo_intr_disestablish");
	}
#endif

	/* Disable it. */
	sys_vme_two->vme_lbint_enable &= ~VME2_LBINT_ENABLE(vec);

	/* Set the interrupt's level to zero */
	ilval &= ~(VME2_ILMASK << ilshift);
	sys_vme_two->vme_irq_levels[ilreg] = ilval;

	/* Clear it */
	sys_vme_two->vme_lbint_clear |= VME2_LBINT_CLEAR(vec);

	/* Un-hook it */
	isrunlink_vectored(vec + VME2_VECTOR_BASE);

	/* Finally, clear the entry in our handler array */
	vme_two_handlers[vec - VME2_VECTOR_MIN].isr_hand = NULL;
	vme_two_handlers[vec - VME2_VECTOR_MIN].isr_arg = NULL;
}

int
vmetwo_translate_addr(start, len, bustype, atype, addrp)
	u_long start;
	size_t len;
	int bustype, atype;
	u_long *addrp;		/* result */
{
	/*
	 * Not yet implemented.
	 *
	 * However, it might be useful to require locators on a 'vmetwo'
	 * device to specify how we should set up the master/slave
	 * windows. We then attach up to four 'vmechip's with locators
	 * specifying the VMEbus window each 'vmechip' refers to. The
	 * traditional 'vmes' and 'vmel' then attach under those.
	 *
	 * Hmm, maybe 'vmechip' should be renamed to 'vmemaster'...
	 * Then we could have 'vmeslave' too ;-)
	 *
	 * Alternatively we just fix things up so the board's view of
	 * the VMEbus is pretty much the same as the MVME-147. 8-)
	 */
	return (1);
}

void
vmetwo_intrline_enable(ipl)
	int ipl;
{
#ifdef DEBUG
	if ( ipl < 1 || ipl > 7 ) {
		printf("vmetwo: Invalid VMEbus IRQ level: %d\n", ipl);
		panic("vmetwo_intrline_enable");
	}
#endif

	sys_vme_two->vme_lbint_enable |= VME2_LBINT_ENABLE(ipl - 1);
}

void
vmetwo_intrline_disable(ipl)
	int ipl;
{
#ifdef DEBUG
	if ( ipl < 1 || ipl > 7 ) {
		printf("vmetwo: Invalid VMEbus IRQ level: %d\n", ipl);
		panic("vmetwo_intrline_disable");
	}
#endif

	sys_vme_two->vme_lbint_enable &= ~VME2_LBINT_ENABLE(ipl - 1);
}

static int
vmetwo_isr_trampoline(arg)
	void *arg;
{
	struct vme_two_handler *isr;
	int vec = (int)arg;
	int s;

#ifdef DEBUG
	if ( vec < VME2_VECTOR_MIN || vec > VME2_VECTOR_MAX ) {
		printf("vmetwo: ISR trampoline: Bad vector 0x%x\n", vec);
		panic("vmetwo_isr_trampoline");
	}
#endif

	/* No interrupts while we fiddle with the registers, please */
	s = splhigh();

	/* Clear the interrupt source */
	sys_vme_two->vme_lbint_clear |= VME2_LBINT_CLEAR(vec);

	splx(s);

	isr = & vme_two_handlers[vec - VME2_VECTOR_MIN];
	if ( isr->isr_hand )
		(*isr->isr_hand)(isr->isr_arg);
	else
		printf("vmetwo: Spurious interrupt, vector 0x%x\n", vec);

	return 1;
}
