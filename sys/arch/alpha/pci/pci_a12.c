/* $NetBSD: pci_a12.c,v 1.1 1998/01/29 21:42:53 ross Exp $ */

/* [Notice revision 2.0]
 * Copyright (c) 1997 Avalon Computer Systems, Inc.
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

__KERNEL_RCSID(0, "$NetBSD: pci_a12.c,v 1.1 1998/01/29 21:42:53 ross Exp $");

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

#ifndef EVCNT_COUNTERS
#include <machine/intrcnt.h>
#endif

int	avalon_a12_intr_map __P((void *, pcitag_t, int, int,
	    pci_intr_handle_t *));
const char *avalon_a12_intr_string __P((void *, pci_intr_handle_t));
void	*avalon_a12_intr_establish __P((void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *));
void	avalon_a12_intr_disestablish __P((void *, void *));

static void clear_gsr_interrupt __P((long));
static void a12_pci_icall __P((int));
static void pci_serr __P((int));
static void a12_xbar_flag __P((int));
static void a12_interval __P((int));
static void a12_check_cdr __P((int));

#ifdef EVCNT_COUNTERS
struct evcnt a12_intr_evcnt;
#endif

static int (*a12_established_device) __P((void *));
static void *ih_arg;

void	a12_iointr __P((void *framep, unsigned long vec));

void
pci_a12_pickintr(ccp)
	struct a12c_config *ccp;
{
	pci_chipset_tag_t pc = &ccp->ac_pc;

        pc->pc_intr_v = ccp;
        pc->pc_intr_map = avalon_a12_intr_map;
        pc->pc_intr_string = avalon_a12_intr_string;
        pc->pc_intr_establish = avalon_a12_intr_establish;
        pc->pc_intr_disestablish = avalon_a12_intr_disestablish;

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

void *
avalon_a12_intr_establish(ccv, ih, level, func, arg)
        void *ccv, *arg;
        pci_intr_handle_t ih;
        int level;
        int (*func) __P((void *));
{
	if(a12_established_device)
		panic("avalon_a12_intr_establish");
	a12_established_device = func;
	ih_arg = arg;
	REGVAL(A12_OMR) |= 1<<7;
	return (void *)func;
}

void
avalon_a12_intr_disestablish(ccv, cookie)
        void *ccv, *cookie;
{
	if(cookie==a12_established_device)
		a12_established_device = 0;
	else	What();
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


static void a12_pci_icall(i)
{
	if(a12_established_device) {
		(*a12_established_device)(ih_arg);
		return;
	}
	printf("PCI irq rcvd, but no handler established.\n");
}

static void pci_serr(i) { panic("pci_serr"); }

static void a12_xbar_flag(i) { panic("a12_xbar_flag"); }

static void a12_interval(i) { panic("a12_interval"); }

static void a12_check_cdr(i)
{
}


static struct gintcall {
	char	flag;
	char	needsclear;
	char	intr_index;	/* XXX implicit crossref */
	void	(*f)(int);
} gintcall[] = {
	{	6,	0,	2,	a12_pci_icall	},
	{	7,	1,	3,	pci_serr	},
	{	8,	1,	4,	a12_xbar_flag	},
	{	9,	1,	5,	a12_xbar_flag	},
		/* skip 10, gsr.TEI */
	{	11,	1,	6,	a12_interval	},
	{	12,	1,	7,	a12_xbar_flag	},
	{	13,	1,	8,	a12_xbar_flag	},
	{	14,	1,	9,	a12_check_cdr	},
	{	0  }
};

static void a12_GInt(void);
static void a12_GInt(void)
{
struct	gintcall *gic;
long	gsrvalue = REGVAL(A12_GSR) & 0x7fc0;

	for(gic=gintcall; gic->f; ++gic) {
		if(gsrvalue & 1L<<gic->flag) {
#ifndef EVCNT_COUNTERS
			if (gic->intr_index >= INTRCNT_A12_IRQ_LEN)
				panic("A12 INTRCNT");
			intrcnt[INTRCNT_A12_IRQ + gic->intr_index];
#endif
			if(gic->needsclear)
				clear_gsr_interrupt(1L<<gic->flag);
			(*gic->f)(gic->flag);
			alpha_wmb();
		}
	}
}

static void a12_xbar_omchint(void);
static void a12_xbar_omchint() { panic("a12_xbar_omchint"); };

static void a12_xbar_imchint(void);
static void a12_xbar_imchint() { panic("a12_xbar_imchint"); };

void
a12_iointr(framep, vec)
	void *framep;
	unsigned long vec;
{
	unsigned irq = (vec-0x900) >> 4;
	/*
	 * Xbar device is in the A12 CPU core logic, so its interrupts
	 * might as well be hardwired.
	 */
#ifdef EVCNT_COUNTERS
	++a12_intr_evcnt.ev_count;
#else   
	/* XXX implicit knowledge of intrcnt[] */
	if(irq < 2)
		++intrcnt[INTRCNT_A12_IRQ + irq];
#if 2 >= INTRCNT_A12_IRQ_LEN
#error	INTRCNT_A12_IRQ_LEN inconsistent with respect to intrcnt[] use
#endif
#endif

	switch(irq) {
	    case 0:
		a12_xbar_omchint();
		return;
	    case 1:
		a12_xbar_imchint();
		return;
	    case 2:
		a12_GInt();
		return;
	    default:
		panic("a12_iointr");
	}
}
