/* $NetBSD: pci_a12.c,v 1.6 2000/06/05 21:47:23 thorpej Exp $ */

/* [Notice revision 2.0]
 * Copyright (c) 1997, 1998 Avalon Computer Systems, Inc.
 * All rights reserved.
 *
 * Author: Ross Harvey
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright and
 *    author notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Avalon Computer Systems, Inc. nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. This copyright will be assigned to The NetBSD Foundation on
 *    1/1/2000 unless these terms (including possibly the assignment
 *    date) are updated in writing by Avalon prior to the latest specified
 *    assignment date.
 *
 * THIS SOFTWARE IS PROVIDED BY AVALON COMPUTER SYSTEMS, INC. AND CONTRIBUTORS
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

#include "opt_avalon_a12.h"		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_a12.c,v 1.6 2000/06/05 21:47:23 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <vm/vm.h>

#include <machine/autoconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/a12creg.h>
#include <alpha/pci/a12cvar.h>

#include <alpha/pci/pci_a12.h>

#define	PCI_A12()	/* Generate ctags(1) key */

#define	LOADGSR() (REGVAL(A12_GSR) & 0x7fc0)

static int pci_serr __P((void *));
static int a12_xbar_flag __P((void *));

struct a12_intr_vect_t {
	int	on;
	int	(*f) __P((void *));
	void	 *a;
} 	a12_intr_pci	= { 0 },
	a12_intr_serr	= { 1, pci_serr },
	a12_intr_flag	= { 1, a12_xbar_flag },
	a12_intr_iei	= { 0 },
	a12_intr_dc	= { 0 },

	a12_intr_xb	= { 0 };

static struct gintcall {
	char	flag;
	char	needsclear;
	char	intr_index;	/* XXX implicit crossref */
	struct	a12_intr_vect_t *f;
	char	*msg;
} gintcall[] = {
	{	6,	0,	2,	&a12_intr_pci,	"PCI Device"	},
	{	7,	1,	3,	&a12_intr_serr,	"PCI SERR#"	},
	{	8,	1,	4,	&a12_intr_flag,	"XB Frame MCE"	},
	{	9,	1,	5,	&a12_intr_flag,	"XB Frame ECE"	},
		/* skip 10, gsr.TEI */
	{	11,	1,	6,	&a12_intr_iei,	"Interval Timer"},
	{	12,	1,	7,	&a12_intr_flag,	"XB FIFO Overrun"},
	{	13,	1,	8,	&a12_intr_flag,	"XB FIFO Underrun"},
	{	14,	1,	9,	&a12_intr_dc,	"DC Control Word"},
	{	0  }
};

struct evcnt a12_intr_evcnt;

int	avalon_a12_intr_map __P((void *, pcitag_t, int, int,
	    pci_intr_handle_t *));
const char *avalon_a12_intr_string __P((void *, pci_intr_handle_t));
const struct evcnt *avalon_a12_intr_evcnt __P((void *, pci_intr_handle_t));
void	*avalon_a12_intr_establish __P((void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *));
void	avalon_a12_intr_disestablish __P((void *, void *));

static void clear_gsr_interrupt __P((long));
static void a12_GInt(void);

void	a12_iointr __P((void *framep, unsigned long vec));

void
pci_a12_pickintr(ccp)
	struct a12c_config *ccp;
{
	pci_chipset_tag_t pc = &ccp->ac_pc;

        pc->pc_intr_v = ccp;
        pc->pc_intr_map = avalon_a12_intr_map;
        pc->pc_intr_string = avalon_a12_intr_string;
	pc->pc_intr_evcnt = avalon_a12_intr_evcnt;
        pc->pc_intr_establish = avalon_a12_intr_establish;
        pc->pc_intr_disestablish = avalon_a12_intr_disestablish;

	/* Not supported on A12. */
	pc->pc_pciide_compat_intr_establish = NULL;

	evcnt_attach_dynamic(&a12_intr_evcnt, EVCNT_TYPE_INTR, NULL,
	    "a12", "pci irq");

	set_iointr(a12_iointr);
}

int     
avalon_a12_intr_map(ccv, bustag, buspin, line, ihp)
        void *ccv;
        pcitag_t bustag; 
        int buspin, line;
        pci_intr_handle_t *ihp;
{
	/* only one PCI slot (per CPU, that is, but there are 12 CPU's!) */
	*ihp = 0;
	return 0;
}

const char *
avalon_a12_intr_string(ccv, ih)
	void *ccv;
	pci_intr_handle_t ih;
{
	return "a12 pci irq";	/* see "only one" note above */
}

const struct evcnt *
avalon_a12_intr_evcnt(ccv, ih)
	void *ccv;
	pci_intr_handle_t ih;
{

	return (&a12_intr_evcnt);
}

void *
avalon_a12_intr_establish(ccv, ih, level, func, arg)
        void *ccv, *arg;
        pci_intr_handle_t ih;
        int level;
        int (*func) __P((void *));
{
	a12_intr_pci.f = func;
	a12_intr_pci.a = arg;
	a12_intr_pci.on= 1;
	alpha_wmb();
	REGVAL(A12_OMR) |= A12_OMR_PCI_ENABLE;
	alpha_mb();
	return (void *)func;
}

void
avalon_a12_intr_disestablish(ccv, cookie)
        void *ccv, *cookie;
{
	if(cookie == a12_intr_pci.f) {
		alpha_wmb();
		REGVAL(A12_OMR) &= ~A12_OMR_PCI_ENABLE;
		alpha_mb();
		a12_intr_pci.f = 0;
		a12_intr_pci.on= 0;
	} else	What();
}

void a12_intr_register_xb(f)
	int (*f) __P((void *));
{
	a12_intr_xb.f  = f;
	a12_intr_xb.on = 1;
}

void a12_intr_register_icw(f)
	int (*f) __P((void *));
{
	long	t;

	t = REGVAL(A12_OMR) & ~A12_OMR_ICW_ENABLE;
	if ((a12_intr_dc.on = (f != NULL)) != 0)
		t |= A12_OMR_ICW_ENABLE;
	a12_intr_dc.f = f;
	alpha_wmb();
	REGVAL(A12_OMR) = t;
	alpha_mb();
}

long	a12_nothing;

static void
clear_gsr_interrupt(write_1_to_clear)
	long	write_1_to_clear;
{
	REGVAL(A12_GSR) = write_1_to_clear;
	alpha_mb();
	a12_nothing = REGVAL(A12_GSR);
}

static int
pci_serr(p)
	void *p;
{
	panic("pci_serr");
}

static int
a12_xbar_flag(p)
	void *p;
{
	panic("a12_xbar_flag: %s", p);
}

static void
a12_GInt(void)
{
struct	gintcall *gic;
long	gsrsource,
	gsrvalue = LOADGSR();
void	*s;

	/*
	 * Switch interrupts do not go through this function
	 */
	for(gic=gintcall; gic->f; ++gic) {
		gsrsource = gsrvalue & 1L<<gic->flag;
		if (gsrsource && gic->f->on) {
			if(gic->needsclear)
				clear_gsr_interrupt(1L<<gic->flag);

			s = gic->f->a;
			if (s == NULL)
				s = gic->msg;
			if (gic->f->f == NULL)
				printf("Stray interrupt: %s OMR=%lx\n",
					gic->msg, REGVAL(A12_OMR) & 0xffc0);
			else	gic->f->f(s);
			alpha_wmb();
		}
	}
}

/*
 *      IRQ_H              3     2     1         0
 *      EV5(10)           23    22    21        20
 *      EV5(16)           17    16    15        14
 *      OSF IPL            6     5     4         3
 *      VECTOR(16)       940   930   920       900      note: no 910 or 940
 *      f(VECTOR)          4     3     2         0      note: no   1 or   4
 *      EVENT           Never  Clk   Misc     Xbar
 */

void
a12_iointr(framep, vec)
	void *framep;
	unsigned long vec;
{
	unsigned irq = (vec-0x900) >> 4;  /* this is the f(vector) above */

	/*
	 * Xbar device is in the A12 CPU core logic, so its interrupts
	 * might as well be hardwired.
	 */
	a12_intr_evcnt.ev_count++;

	switch(irq) {
	    case 0:
		if (a12_intr_xb.f == NULL)
			panic("no switch interrupt registered");
		else	a12_intr_xb.f(NULL);
		return;
	    case 2:
		a12_GInt();
		return;
	    default:
		panic("a12_iointr");
	}
}
