/*	$NetBSD: isa_machdep.c,v 1.1.1.1 1997/07/15 08:17:39 leo Exp $	*/

/*
 * Copyright (c) 1997 Leo Weppelman.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
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

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <atari/atari/device.h>

static int	isabusprint __P((void *auxp, const char *));
static int	isabusmatch __P((struct device *, struct cfdata *, void *));
static void	isabusattach __P((struct device *, struct device *, void *));

struct cfattach isabus_ca = {
	sizeof(struct device), isabusmatch, isabusattach
};

struct cfdriver isabus_cd = {
	NULL, "isabus", DV_DULL
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
	struct isabus_attach_args	iba;

	iba.iba_busname = "isa";
	iba.iba_iot     = ISA_IOSTART;
	iba.iba_memt    = ISA_MEMSTART;

	MFP->mf_aer    |= (IO_ISA1|IO_ISA2); /* ISA interrupts: LOW->HIGH */

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
	if (slot == 1)
		MFP->mf_imrb  &= ~(IB_ISA1);
	else MFP->mf_imra &= ~(IA_ISA2);


	if ((sr & PSL_IPL) >= iinfo_p->ipl) {
		/*
		 * We're running at a too high priority now.
		 */
		add_sicallback((si_farg)iifun, (void*)slot, 0);
	}
	else {
		s = splx(iinfo_p->ipl);
		(void) (iinfo_p->ifunc)(iinfo_p->iarg, 0);
		splx(s);

		/*
		 * Re-enable interrupts after handling
		 */
		if (slot == 1)
			MFP->mf_imrb  |= IB_ISA1;
		else MFP->mf_imra |= IA_ISA2;
	}
	return 1;
}

void *
isa_intr_establish(ic, irq, type, level, ih_fun, ih_arg)
	isa_chipset_tag_t ic;
	int		  irq, type, level;
	hw_ifun_t	  ih_fun;
	void		  *ih_arg;
{
	isa_intr_info_t *iinfo_p;
	struct intrhand	*ihand;
	int		slot;

printf("isa_intr_est: irq:%d, type:%d, level:%d\n",irq,type,level);
	slot    = (irq <= 6) ? 1 : 2;
	iinfo_p = &iinfo[slot];

	if (iinfo_p->slot > 0)
	    panic("isa_intr_establish: interrupt was already established\n");

	ihand = intr_establish((slot == 1) ? 3 : 15, USER_VEC, 0,
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
		if (slot == 1) {
			MFP->mf_imrb |= IB_ISA1;
			MFP->mf_ierb |= IB_ISA1;
		}
		else {
			MFP->mf_imra |= IA_ISA2;
			MFP->mf_iera |= IA_ISA2;
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
