/*	$NetBSD: isa_machdep.c,v 1.13.4.1 1999/11/15 00:37:31 fvdl Exp $	*/

/*
 * Copyright (c) 1997 Leo Weppelman.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>

#include <m68k/asm_single.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <atari/atari/device.h>

static int	isabusprint __P((void *auxp, const char *));
static int	isabusmatch __P((struct device *, struct cfdata *, void *));
static void	isabusattach __P((struct device *, struct device *, void *));

struct isabus_softc {
	struct device sc_dev;
	struct atari_isa_chipset sc_chipset;
};

struct cfattach isabus_ca = {
	sizeof(struct isabus_softc), isabusmatch, isabusattach
};

int
isabusmatch(pdp, cfp, auxp)
struct device	*pdp;
struct cfdata	*cfp;
void		*auxp;
{
	if(atari_realconfig == 0)
		return (0);
	if (strcmp((char *)auxp, "isabus") || cfp->cf_unit != 0)
		return(0);
	return(machineid & ATARI_HADES ? 1 : 0);
}

void
isabusattach(pdp, dp, auxp)
struct device	*pdp, *dp;
void		*auxp;
{
	struct isabus_softc *sc = (struct isabus_softc *)dp;
	struct isabus_attach_args	iba;
	bus_space_tag_t			leb_alloc_bus_space_tag __P((void));

	iba.iba_busname = "isa";
	iba.iba_dmat	= BUS_ISA_DMA_TAG;
	iba.iba_iot     = leb_alloc_bus_space_tag();
	iba.iba_memt    = leb_alloc_bus_space_tag();
	iba.iba_ic	= &sc->sc_chipset;
	if ((iba.iba_iot == NULL) || (iba.iba_memt == NULL)) {
		printf("leb_alloc_bus_space_tag failed!\n");
		return;
	}
	iba.iba_iot->base  = ISA_IOSTART;
	iba.iba_memt->base = ISA_MEMSTART;

	MFP->mf_aer |= (IO_ISA1|IO_ISA2); /* ISA interrupts: LOW->HIGH */

	printf("\n");
	config_found(dp, &iba, isabusprint);
}

int
isabusprint(auxp, name)
void		*auxp;
const char	*name;
{
	if(name == NULL)
		return(UNCONF);
	return(QUIET);
}

void
isa_attach_hook(parent, self, iba)
	struct device		  *parent, *self;
	struct isabus_attach_args *iba;
{
}

/*
 * The interrupt stuff is rather ugly. On the Hades, all interrupt lines
 * for a slot are wired together and connected to either IO3 (slot1) or
 * IO7 (slot2). Since no info can be obtained about the slot position
 * at isa_intr_establish() time, the following assumption is made:
 *   - irq <= 6 -> slot 1
 *   - irq >  6 -> slot 2
 */

#define	SLOTNR(irq)	((irq <= 6) ? 0 : 1)

static isa_intr_info_t iinfo[2] = { { -1 }, { -1 } };

static int	iifun __P((int, int));

static int
iifun(slot, sr)
int	slot;
int	sr;
{
	isa_intr_info_t *iinfo_p;
	int		s;

	iinfo_p = &iinfo[slot];

	/*
	 * Disable the interrupts
	 */
	if (slot == 0) {
		single_inst_bclr_b(MFP->mf_imrb, IB_ISA1);
	}
	else {
		single_inst_bclr_b(MFP->mf_imra, IA_ISA2);
	}

	if ((sr & PSL_IPL) >= (iinfo_p->ipl & PSL_IPL)) {
		/*
		 * We're running at a too high priority now.
		 */
		add_sicallback((si_farg)iifun, (void*)slot, 0);
	}
	else {
		s = splx(iinfo_p->ipl);
		if (slot == 0) {
			do {
				MFP->mf_iprb = (u_int8_t)~IB_ISA1;
				(void) (iinfo_p->ifunc)(iinfo_p->iarg);
			} while (MFP->mf_iprb & IB_ISA1);
			single_inst_bset_b(MFP->mf_imrb, IB_ISA1);
		}
		else {
			do {
				MFP->mf_ipra = (u_int8_t)~IA_ISA2;
				(void) (iinfo_p->ifunc)(iinfo_p->iarg);
			} while (MFP->mf_ipra & IA_ISA2);
			single_inst_bset_b(MFP->mf_imra, IA_ISA2);
		}
		splx(s);
	}
	return 1;
}


/*
 * XXXX
 * XXXX Note that this function is not really working yet! The big problem is
 * XXXX to only generate interrupts for the slot the card is in...
 */
int
isa_intr_alloc(ic, mask, type, irq)
	isa_chipset_tag_t ic;
	int mask;
	int type;
	int *irq;
{
	isa_intr_info_t *iinfo_p;
	int		slot, i;


	/*
	 * The Hades only supports edge triggered interrupts!
	 */
	if (type != IST_EDGE)
		return 1;

#define	MAXIRQ		10	/* XXX: Pure fiction	*/
	for (i = 0; i < MAXIRQ; i++) {
		if (mask & (1<<i)) {
		    slot    = SLOTNR(i);
		    iinfo_p = &iinfo[slot];

		    if (iinfo_p->slot >= 0) {
			*irq = i;
			printf("WARNING: isa_intr_alloc is not yet ready!\n");
			return 0;
		    }
		}
	}
	return (1);
}

void *
isa_intr_establish(ic, irq, type, level, ih_fun, ih_arg)
	isa_chipset_tag_t ic;
	int		  irq, type, level;
	int		  (*ih_fun) __P((void *));
	void		  *ih_arg;
{
	isa_intr_info_t *iinfo_p;
	struct intrhand	*ihand;
	int		slot;

	/*
	 * The Hades only supports edge triggered interrupts!
	 */
	if (type != IST_EDGE)
		return NULL;

	slot    = SLOTNR(irq);
	iinfo_p = &iinfo[slot];

	if (iinfo_p->slot >= 0)
	    panic("isa_intr_establish: interrupt was already established\n");

	ihand = intr_establish((slot == 0) ? 3 : 15, USER_VEC, 0,
				(hw_ifun_t)iifun, (void *)slot);
	if (ihand != NULL) {
		iinfo_p->slot  = slot;
		iinfo_p->ipl   = level;
		iinfo_p->ifunc = ih_fun;
		iinfo_p->iarg  = ih_arg;
		iinfo_p->ihand = ihand;

		/*
		 * Enable (unmask) the interrupt
		 */
		if (slot == 0) {
			single_inst_bset_b(MFP->mf_imrb, IB_ISA1);
			single_inst_bset_b(MFP->mf_ierb, IB_ISA1);
		}
		else {
			single_inst_bset_b(MFP->mf_imra, IA_ISA2);
			single_inst_bset_b(MFP->mf_iera, IA_ISA2);
		}
		return(iinfo_p);
	}
	return NULL;
}

void
isa_intr_disestablish(ic, handler)
	isa_chipset_tag_t	ic;
	void			*handler;
{
	isa_intr_info_t *iinfo_p = (isa_intr_info_t *)handler;

	if (iinfo_p->slot < 0)
	    panic("isa_intr_disestablish: interrupt was not established\n");

	(void) intr_disestablish(iinfo_p->ihand);
	iinfo_p->slot = -1;
}
