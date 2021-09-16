/* $NetBSD: pci_machdep.c,v 1.33 2021/09/16 20:17:46 andvar Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Machine-specific functions for PCI autoconfiguration.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.33 2021/09/16 20:17:46 andvar Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/cpu.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include "vga_pci.h"
#if NVGA_PCI
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/pci/vga_pcivar.h>
#endif

#include "tga.h"
#if NTGA
#include <dev/pci/tgavar.h>
#endif

#include <machine/rpb.h>

void
pci_display_console(bus_space_tag_t iot, bus_space_tag_t memt, pci_chipset_tag_t pc, int bus, int device, int function)
{
#if NVGA_PCI || NTGA
	pcitag_t tag;
	pcireg_t id;
	int match, nmatch;
#endif
#if NVGA_PCI
	pcireg_t class;
#endif
	int (*fn)(bus_space_tag_t, bus_space_tag_t, pci_chipset_tag_t,
	    int, int, int);

#if NVGA_PCI || NTGA
	tag = pci_make_tag(pc, bus, device, function);
	id = pci_conf_read(pc, tag, PCI_ID_REG);
	if (id == 0 || id == 0xffffffff)
		panic("pci_display_console: no device at %d/%d/%d",
		    bus, device, function);
#  if NVGA_PCI
	class = pci_conf_read(pc, tag, PCI_CLASS_REG);
#  endif

	match = 0;
#endif
	fn = NULL;

#if NVGA_PCI
	nmatch = DEVICE_IS_VGA_PCI(class, id);
	if (nmatch > match) {
		match = nmatch;
		fn = vga_pci_cnattach;
	}
#endif
#if NTGA
	nmatch = DEVICE_IS_TGA(class, id);
	if (nmatch > match)
		nmatch = tga_cnmatch(iot, memt, pc, tag);
	if (nmatch > match) {
		match = nmatch;
		fn = tga_cnattach;
	}
#endif

	if (fn != NULL)
		(*fn)(iot, memt, pc, bus, device, function);
	else
		panic("pci_display_console: unconfigured device at %d/%d/%d",
		    bus, device, function);
}

void
device_pci_register(device_t dev, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct ctb *ctb;
	prop_dictionary_t dict;

	/* set properties for PCI framebuffers */
	ctb = (struct ctb *)(((char *)hwrpb) + hwrpb->rpb_ctb_off);
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY &&
	    ctb->ctb_term_type == CTB_GRAPHICS) {
		/* XXX should consider multiple displays? */
		dict = device_properties(dev);
		prop_dictionary_set_bool(dict, "is_console", true);
	}
}

void
alpha_pci_intr_init(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{
	__link_set_decl(alpha_pci_intr_impls, struct alpha_pci_intr_impl);
	struct alpha_pci_intr_impl * const *impl;

	__link_set_foreach(impl, alpha_pci_intr_impls) {
		if ((*impl)->systype == cputype) {
			(*impl)->intr_init(core, iot, memt, pc);
			return;
		}
	}
	panic("%s: unknown systype %d", __func__, cputype);
}

void
alpha_pci_intr_alloc(pci_chipset_tag_t pc, unsigned int maxstrays)
{
	unsigned int i;
	struct evcnt *ev;
	const char *cp;

	pc->pc_shared_intrs = alpha_shared_intr_alloc(pc->pc_nirq);

	for (i = 0; i < pc->pc_nirq; i++) {
		alpha_shared_intr_set_maxstrays(pc->pc_shared_intrs, i,
		    maxstrays);
		alpha_shared_intr_set_private(pc->pc_shared_intrs, i,
		    pc->pc_intr_v);

		ev = alpha_shared_intr_evcnt(pc->pc_shared_intrs, i);
		cp = alpha_shared_intr_string(pc->pc_shared_intrs, i);

		evcnt_attach_dynamic(ev, EVCNT_TYPE_INTR, NULL,
		    pc->pc_intr_desc, cp);
	}
}

int
alpha_pci_generic_intr_map(const struct pci_attach_args * const pa,
    pci_intr_handle_t * const ihp)
{
	pcitag_t const bustag = pa->pa_intrtag;
	int const buspin = pa->pa_intrpin;
	int const line = pa->pa_intrline;
	pci_chipset_tag_t const pc = pa->pa_pc;
	int bus, device, function;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin < 0 || buspin > 4) {
		printf("%s: bad interrupt pin %d\n", __func__, buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, &bus, &device, &function);

	/*
	 * The console firmware places the interrupt mapping in the "line"
	 * value.  A valaue of (char)-1 indicates there is no mapping.
	 */
	if (line == 0xff) {
		printf("%s: no mapping for %d/%d/%d\n", __func__,
		    bus, device, function);
		return 1;
	}

	if (line < 0 || line >= pc->pc_nirq) {
		printf("%s: bad line %d for %d/%d/%d\n", __func__,
		    line, bus, device, function);
		return 1;
	}

	alpha_pci_intr_handle_init(ihp, line, 0);
	return 0;
}

const char *
alpha_pci_generic_intr_string(pci_chipset_tag_t const pc,
    pci_intr_handle_t const ih, char * const buf, size_t const len)
{
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);

	KASSERT(irq < pc->pc_nirq);

	snprintf(buf, len, "%s irq %u", pc->pc_intr_desc, irq);
	return buf;
}

const struct evcnt *
alpha_pci_generic_intr_evcnt(pci_chipset_tag_t const pc,
    pci_intr_handle_t const ih)
{
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);

	KASSERT(irq < pc->pc_nirq);

	return alpha_shared_intr_evcnt(pc->pc_shared_intrs, irq);
}

static struct cpu_info *
alpha_pci_generic_intr_select_cpu(pci_chipset_tag_t const pc, u_int const irq,
    u_int const flags)
{
	struct cpu_info *ci, *best_ci;
	CPU_INFO_ITERATOR cii;

	KASSERT(mutex_owned(&cpu_lock));

	/*
	 * If the back-end didn't tell us where we can route, then
	 * they all go to the primary CPU.
	 */
	if (pc->pc_eligible_cpus == 0) {
		return &cpu_info_primary;
	}

	/*
	 * If the interrupt already has a CPU assigned, keep on using it,
	 * unless the CPU has become ineligible.
	 */
	ci = alpha_shared_intr_get_cpu(pc->pc_shared_intrs, irq);
	if (ci != NULL) {
		if ((ci->ci_schedstate.spc_flags & SPCF_NOINTR) == 0 ||
		    CPU_IS_PRIMARY(ci)) {
			return ci;
		}
	}

	/*
	 * Pick the CPU with the fewest handlers.
	 */
	best_ci = NULL;
	for (CPU_INFO_FOREACH(cii, ci)) {
		if ((pc->pc_eligible_cpus & __BIT(ci->ci_cpuid)) == 0) {
			/* This CPU is not eligible in hardware. */
			continue;
		}
		if (ci->ci_schedstate.spc_flags & SPCF_NOINTR) {
			/* This CPU is not eligible in software. */
			continue;
		}
		if (best_ci == NULL ||
		    ci->ci_nintrhand < best_ci->ci_nintrhand) {
			best_ci = ci;
		}
	}

	/* If we found one, cool... */
	if (best_ci != NULL) {
		return best_ci;
	}

	/* ...if not, well I guess we'll just fall back on the primary. */
	return &cpu_info_primary;
}

void *
alpha_pci_generic_intr_establish(pci_chipset_tag_t const pc,
    pci_intr_handle_t const ih, int const level,
    int (*func)(void *), void *arg)
{
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);
	const u_int flags = alpha_pci_intr_handle_get_flags(&ih);
	void *cookie;

	KASSERT(irq < pc->pc_nirq);

	cookie = alpha_shared_intr_alloc_intrhand(pc->pc_shared_intrs,
	    irq, IST_LEVEL, level, flags, func, arg, pc->pc_intr_desc);

	if (cookie == NULL)
		return NULL;

	mutex_enter(&cpu_lock);

	struct cpu_info *target_ci =
	    alpha_pci_generic_intr_select_cpu(pc, irq, flags);
	struct cpu_info *current_ci =
	    alpha_shared_intr_get_cpu(pc->pc_shared_intrs, irq);

	const bool first_handler =
	    ! alpha_shared_intr_isactive(pc->pc_shared_intrs, irq);

	/*
	 * If this is the first handler on this interrupt, or if the
	 * target CPU has changed, then program the route if the
	 * hardware supports it.
	 */
	if (first_handler || target_ci != current_ci) {
		alpha_shared_intr_set_cpu(pc->pc_shared_intrs, irq, target_ci);
		if (pc->pc_intr_set_affinity != NULL) {
			pc->pc_intr_set_affinity(pc, irq, target_ci);
		}
	}

	if (! alpha_shared_intr_link(pc->pc_shared_intrs, cookie,
				     pc->pc_intr_desc)) {
		mutex_exit(&cpu_lock);
		alpha_shared_intr_free_intrhand(cookie);
		return NULL;
	}

	if (first_handler) {
		scb_set(pc->pc_vecbase + SCB_IDXTOVEC(irq),
		    alpha_pci_generic_iointr, pc);
		pc->pc_intr_enable(pc, irq);
	}

	mutex_exit(&cpu_lock);

	return cookie;
}

void
alpha_pci_generic_intr_disestablish(pci_chipset_tag_t const pc,
    void * const cookie)
{
	struct alpha_shared_intrhand * const ih = cookie;
	const u_int irq = ih->ih_num;

	mutex_enter(&cpu_lock);

	if (alpha_shared_intr_firstactive(pc->pc_shared_intrs, irq)) {
		pc->pc_intr_disable(pc, irq);
		alpha_shared_intr_set_dfltsharetype(pc->pc_shared_intrs,
		    irq, IST_NONE);
		scb_free(pc->pc_vecbase + SCB_IDXTOVEC(irq));
	}

	alpha_shared_intr_unlink(pc->pc_shared_intrs, cookie, pc->pc_intr_desc);

	mutex_exit(&cpu_lock);

	alpha_shared_intr_free_intrhand(cookie);
}

void
alpha_pci_generic_iointr(void * const arg, unsigned long const vec)
{
	pci_chipset_tag_t const pc = arg;
	const u_int irq = SCB_VECTOIDX(vec - pc->pc_vecbase);

	if (!alpha_shared_intr_dispatch(pc->pc_shared_intrs, irq)) {
		alpha_shared_intr_stray(pc->pc_shared_intrs, irq,
		    pc->pc_intr_desc);
		if (ALPHA_SHARED_INTR_DISABLE(pc->pc_shared_intrs, irq)) {
			pc->pc_intr_disable(pc, irq);
		}
	} else {
		alpha_shared_intr_reset_strays(pc->pc_shared_intrs, irq);
	}
}

void
alpha_pci_generic_intr_redistribute(pci_chipset_tag_t const pc)
{
	struct cpu_info *current_ci, *new_ci;
	unsigned int irq;

	KASSERT(mutex_owned(&cpu_lock));
	KASSERT(mp_online);

	/* If we can't set affinity, then there's nothing to do. */
	if (pc->pc_eligible_cpus == 0 || pc->pc_intr_set_affinity == NULL) {
		return;
	}

	/*
	 * Look at each IRQ, and allocate a new CPU for each IRQ
	 * that's being serviced by a now-shielded CPU.
	 */
	for (irq = 0; irq < pc->pc_nirq; irq++) {
		current_ci =
		    alpha_shared_intr_get_cpu(pc->pc_shared_intrs, irq);
		if (current_ci == NULL ||
		    (current_ci->ci_schedstate.spc_flags & SPCF_NOINTR) == 0) {
			continue;
		}

		new_ci = alpha_pci_generic_intr_select_cpu(pc, irq, 0);
		if (new_ci == current_ci) {
			/* Can't shield this one. */
			continue;
		}

		alpha_shared_intr_set_cpu(pc->pc_shared_intrs, irq, new_ci);
		pc->pc_intr_set_affinity(pc, irq, new_ci);
	}

	/* XXX should now re-balance */
}

#define	ALPHA_PCI_INTR_HANDLE_IRQ	__BITS(0,31)
#define	ALPHA_PCI_INTR_HANDLE_FLAGS	__BITS(32,63)

void
alpha_pci_intr_handle_init(pci_intr_handle_t * const ihp, u_int const irq,
    u_int const flags)
{
	ihp->value = __SHIFTIN(irq, ALPHA_PCI_INTR_HANDLE_IRQ) |
	    __SHIFTIN(flags, ALPHA_PCI_INTR_HANDLE_FLAGS);
}

void
alpha_pci_intr_handle_set_irq(pci_intr_handle_t * const ihp, u_int const irq)
{
	ihp->value = (ihp->value & ALPHA_PCI_INTR_HANDLE_FLAGS) |
	    __SHIFTIN(irq, ALPHA_PCI_INTR_HANDLE_IRQ);
}

u_int
alpha_pci_intr_handle_get_irq(const pci_intr_handle_t * const ihp)
{
	return __SHIFTOUT(ihp->value, ALPHA_PCI_INTR_HANDLE_IRQ);
}

void
alpha_pci_intr_handle_set_flags(pci_intr_handle_t * const ihp,
    u_int const flags)
{
	ihp->value = (ihp->value & ALPHA_PCI_INTR_HANDLE_IRQ) |
	    __SHIFTIN(flags, ALPHA_PCI_INTR_HANDLE_FLAGS);
}

u_int
alpha_pci_intr_handle_get_flags(const pci_intr_handle_t * const ihp)
{
	return __SHIFTOUT(ihp->value, ALPHA_PCI_INTR_HANDLE_FLAGS);
}

/*
 * MI PCI back-end entry points.
 */

void
pci_attach_hook(device_t const parent, device_t const self,
    struct pcibus_attach_args * const pba)
{
	pci_chipset_tag_t const pc = pba->pba_pc;

	if (pc->pc_attach_hook != NULL) {
		pc->pc_attach_hook(parent, self, pba);
	}
}

int
pci_bus_maxdevs(pci_chipset_tag_t const pc, int const busno)
{
	if (pc->pc_bus_maxdevs == NULL) {
		return 32;
	}

	return pc->pc_bus_maxdevs(pc->pc_conf_v, busno);
}

pcitag_t
pci_make_tag(pci_chipset_tag_t const pc, int const bus, int const dev,
    int const func)
{
	if (__predict_true(pc->pc_make_tag == NULL)) {
		/* Just use the standard Type 1 address format. */
		return __SHIFTIN(bus, PCI_CONF_TYPE1_BUS) |
		       __SHIFTIN(dev, PCI_CONF_TYPE1_DEVICE) |
		       __SHIFTIN(func, PCI_CONF_TYPE1_FUNCTION);
	}

	return pc->pc_make_tag(pc->pc_conf_v, bus, dev, func);
}

void
pci_decompose_tag(pci_chipset_tag_t const pc, pcitag_t const tag,
    int * const busp, int * const devp, int * const funcp)
{
	if (__predict_true(pc->pc_decompose_tag == NULL)) {
		if (busp != NULL)
			*busp = __SHIFTOUT(tag, PCI_CONF_TYPE1_BUS);
		if (devp != NULL)
			*devp = __SHIFTOUT(tag, PCI_CONF_TYPE1_DEVICE);
		if (funcp != NULL)
			*funcp = __SHIFTOUT(tag, PCI_CONF_TYPE1_FUNCTION);
		return;
	}

	pc->pc_decompose_tag(pc->pc_conf_v, tag, busp, devp, funcp);
}

pcireg_t
pci_conf_read(pci_chipset_tag_t const pc, pcitag_t const tag, int const reg)
{
	KASSERT(pc->pc_conf_read != NULL);
	return pc->pc_conf_read(pc->pc_conf_v, tag, reg);
}

void
pci_conf_write(pci_chipset_tag_t const pc, pcitag_t const tag, int const reg,
    pcireg_t const val)
{
	KASSERT(pc->pc_conf_write != NULL);
	pc->pc_conf_write(pc->pc_conf_v, tag, reg, val);
}

int
pci_intr_map(const struct pci_attach_args * const pa,
    pci_intr_handle_t * const ihp)
{
	pci_chipset_tag_t const pc = pa->pa_pc;

	KASSERT(pc->pc_intr_map != NULL);
	return pc->pc_intr_map(pa, ihp);
}

const char *
pci_intr_string(pci_chipset_tag_t const pc, pci_intr_handle_t const ih,
    char * const buf, size_t const len)
{
	KASSERT(pc->pc_intr_string != NULL);
	return pc->pc_intr_string(pc, ih, buf, len);
}

const struct evcnt *
pci_intr_evcnt(pci_chipset_tag_t const pc, pci_intr_handle_t const ih)
{
	KASSERT(pc->pc_intr_evcnt != NULL);
	return pc->pc_intr_evcnt(pc, ih);
}

void *
pci_intr_establish(pci_chipset_tag_t const pc, pci_intr_handle_t const ih,
    int const ipl, int (*func)(void *), void *arg)
{
	KASSERT(pc->pc_intr_establish != NULL);
	return pc->pc_intr_establish(pc, ih, ipl, func, arg);
}

void
pci_intr_disestablish(pci_chipset_tag_t const pc, void * const cookie)
{
	KASSERT(pc->pc_intr_disestablish != NULL);
	pc->pc_intr_disestablish(pc, cookie);
}

int
pci_intr_setattr(pci_chipset_tag_t const pc __unused,
    pci_intr_handle_t * const ihp, int const attr, uint64_t const data)
{
	u_int flags = alpha_pci_intr_handle_get_flags(ihp);

	switch (attr) {
	case PCI_INTR_MPSAFE:
		if (data)
			flags |= ALPHA_INTR_MPSAFE;
		else
			flags &= ~ALPHA_INTR_MPSAFE;
		break;
	
	default:
		return ENODEV;
	}

	alpha_pci_intr_handle_set_flags(ihp, flags);
	return 0;
}
