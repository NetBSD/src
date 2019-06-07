/*	$NetBSD: xen_intr.c,v 1.17 2019/06/07 12:43:52 cherry Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xen_intr.c,v 1.17 2019/06/07 12:43:52 cherry Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <sys/cpu.h>

#include <xen/evtchn.h>
#include <xen/xenfunc.h>

#include <uvm/uvm.h>

#include <machine/cpu.h>
#include <machine/intr.h>

#include "acpica.h"
#include "ioapic.h"
#include "lapic.h"
#include "pci.h"

#if NACPICA > 0
#include <dev/acpi/acpivar.h>
#endif

#if NIOAPIC > 0 || NACPICA > 0
#include <machine/i82093var.h>
#endif

#if NLAPIC > 0
#include <machine/i82489var.h>
#endif

#if NPCI > 0
#include <dev/pci/ppbreg.h>
#endif

/*
 * Restore a value to cpl (unmasking interrupts).  If any unmasked
 * interrupts are pending, call Xspllower() to process them.
 */
void xen_spllower(int nlevel);

void
xen_spllower(int nlevel)
{
	struct cpu_info *ci = curcpu();
	uint32_t xmask;
	u_long psl;

	if (ci->ci_ilevel <= nlevel)
		return;

	__insn_barrier();

	xmask = XUNMASK(ci, nlevel);
	psl = xen_read_psl();
	x86_disable_intr();
	if (ci->ci_xpending & xmask) {
		KASSERT(psl == 0);
		Xspllower(nlevel);
		/* Xspllower does enable_intr() */
	} else {
		ci->ci_ilevel = nlevel;
		xen_write_psl(psl);
	}
}


#if !defined(XENPVHVM)
void
x86_disable_intr(void)
{
	curcpu()->ci_vcpu->evtchn_upcall_mask = 1;
	x86_lfence();
}

void
x86_enable_intr(void)
{
	volatile struct vcpu_info *_vci = curcpu()->ci_vcpu;
	__insn_barrier();
	_vci->evtchn_upcall_mask = 0;
	x86_lfence(); /* unmask then check (avoid races) */
	if (__predict_false(_vci->evtchn_upcall_pending))
		hypervisor_force_callback();
}

#endif /* !XENPVHVM */

u_long
xen_read_psl(void)
{

	return (curcpu()->ci_vcpu->evtchn_upcall_mask);
}

void
xen_write_psl(u_long psl)
{
	struct cpu_info *ci = curcpu();

	ci->ci_vcpu->evtchn_upcall_mask = psl;
	xen_rmb();
	if (ci->ci_vcpu->evtchn_upcall_pending && psl == 0) {
	    	hypervisor_force_callback();
	}
}

void *
xen_intr_establish(int legacy_irq, struct pic *pic, int pin,
    int type, int level, int (*handler)(void *), void *arg,
    bool known_mpsafe)
{

	return xen_intr_establish_xname(legacy_irq, pic, pin, type, level,
	    handler, arg, known_mpsafe, "XEN");
}

void *
xen_intr_establish_xname(int legacy_irq, struct pic *pic, int pin,
    int type, int level, int (*handler)(void *), void *arg,
    bool known_mpsafe, const char *xname)
{
	const char *intrstr;
	char intrstr_buf[INTRIDBUF];

	if (pic->pic_type == PIC_XEN) {
		struct intrhand *rih;

		/*
		 * event_set_handler interprets `level != IPL_VM' to
		 * mean MP-safe, so we require the caller to match that
		 * for the moment.
		 */
		KASSERT(known_mpsafe == (level != IPL_VM));

		intrstr = intr_create_intrid(legacy_irq, pic, pin, intrstr_buf,
		    sizeof(intrstr_buf));

		event_set_handler(pin, handler, arg, level, intrstr, xname);

		rih = kmem_zalloc(sizeof(*rih), cold ? KM_NOSLEEP : KM_SLEEP);
		if (rih == NULL) {
			printf("%s: can't allocate handler info\n", __func__);
			return NULL;
		}

		/*
		 * XXX:
		 * This is just a copy for API conformance.
		 * The real ih is lost in the innards of
		 * event_set_handler(); where the details of
		 * biglock_wrapper etc are taken care of.
		 * All that goes away when we nuke event_set_handler()
		 * et. al. and unify with x86/intr.c
		 */
		rih->ih_pin = pin; /* port */
		rih->ih_fun = rih->ih_realfun = handler;
		rih->ih_arg = rih->ih_realarg = arg;
		rih->pic_type = pic->pic_type;
		return rih;
	} 	/* Else we assume pintr */

#if (NPCI > 0 || NISA > 0) && defined(XENPV) /* XXX: support PVHVM pirq */
	struct pintrhand *pih;
	int gsi;
	int vector, evtchn;

	KASSERTMSG(legacy_irq == -1 || (0 <= legacy_irq && legacy_irq < NUM_XEN_IRQS),
	    "bad legacy IRQ value: %d", legacy_irq);
	KASSERTMSG(!(legacy_irq == -1 && pic == &i8259_pic),
	    "non-legacy IRQon i8259 ");

	gsi = xen_pic_to_gsi(pic, pin);

	intrstr = intr_create_intrid(gsi, pic, pin, intrstr_buf,
	    sizeof(intrstr_buf));

	vector = xen_vec_alloc(gsi);

	if (irq2port[gsi] == 0) {
		extern struct cpu_info phycpu_info_primary; /* XXX */
		struct cpu_info *ci = &phycpu_info_primary;

		pic->pic_addroute(pic, ci, pin, vector, type);

		evtchn = bind_pirq_to_evtch(gsi);
		KASSERT(evtchn > 0);
		KASSERT(evtchn < NR_EVENT_CHANNELS);
		irq2port[gsi] = evtchn + 1;
		xen_atomic_set_bit(&ci->ci_evtmask[0], evtchn);
	} else {
		/*
		 * Shared interrupt - we can't rebind.
		 * The port is shared instead.
		 */
		evtchn = irq2port[gsi] - 1;
	}

	pih = pirq_establish(gsi, evtchn, handler, arg, level,
			     intrstr, xname);
	pih->pic_type = pic->pic_type;
	return pih;
#endif /* NPCI > 0 || NISA > 0 */

	/* FALLTHROUGH */
	return NULL;
}

/*
 * Deregister an interrupt handler.
 */
void
xen_intr_disestablish(struct intrhand *ih)
{

	if (ih->pic_type == PIC_XEN) {
		event_remove_handler(ih->ih_pin, ih->ih_realfun,
		    ih->ih_realarg);
		kmem_free(ih, sizeof(*ih));
		return;
	}
#if defined(DOM0OPS)
	/* 
	 * Cache state, to prevent a use after free situation with
	 * ih.
	 */

	struct pintrhand *pih = (struct pintrhand *)ih;

	int pirq = pih->pirq;
	int port = pih->evtch;
	KASSERT(irq2port[pirq] != 0);

	pirq_disestablish(pih);

	if (evtsource[port] == NULL) {
			/*
			 * Last handler was removed by
			 * event_remove_handler().
			 *
			 * We can safely unbind the pirq now.
			 */

			port = unbind_pirq_from_evtch(pirq);
			KASSERT(port == pih->evtch);
			irq2port[pirq] = 0;
	}
#endif
	return;
}

/* MI interface for kern_cpu.c */
void xen_cpu_intr_redistribute(void);

void
xen_cpu_intr_redistribute(void)
{
	KASSERT(mutex_owned(&cpu_lock));
	KASSERT(mp_online);

	return;
}

/* MD - called by x86/cpu.c */
#if defined(INTRSTACKSIZE)
static inline bool
redzone_const_or_false(bool x)
{
#ifdef DIAGNOSTIC
	return x;
#else
	return false;
#endif /* !DIAGNOSTIC */
}

static inline int
redzone_const_or_zero(int x)
{
	return redzone_const_or_false(true) ? x : 0;
}
#endif

void xen_cpu_intr_init(struct cpu_info *);
void
xen_cpu_intr_init(struct cpu_info *ci)
{
	int i; /* XXX: duplicate */

	ci->ci_xunmask[0] = 0xfffffffe;
	for (i = 1; i < NIPL; i++)
		ci->ci_xunmask[i] = ci->ci_xunmask[i - 1] & ~(1 << i);

#if defined(INTRSTACKSIZE)
	vaddr_t istack;

	/*
	 * If the red zone is activated, protect both the top and
	 * the bottom of the stack with an unmapped page.
	 */
	istack = uvm_km_alloc(kernel_map,
	    INTRSTACKSIZE + redzone_const_or_zero(2 * PAGE_SIZE), 0,
	    UVM_KMF_WIRED|UVM_KMF_ZERO);
	if (redzone_const_or_false(true)) {
		pmap_kremove(istack, PAGE_SIZE);
		pmap_kremove(istack + INTRSTACKSIZE + PAGE_SIZE, PAGE_SIZE);
		pmap_update(pmap_kernel());
	}

	/*
	 * 33 used to be 1.  Arbitrarily reserve 32 more register_t's
	 * of space for ddb(4) to examine some subroutine arguments
	 * and to hunt for the next stack frame.
	 */
	ci->ci_intrstack = (char *)istack + redzone_const_or_zero(PAGE_SIZE) +
	    INTRSTACKSIZE - 33 * sizeof(register_t);
#endif

	ci->ci_idepth = -1;
}

/*
 * Everything below from here is duplicated from x86/intr.c
 * When intr.c and xen_intr.c are unified, these will need to be
 * merged.
 */

u_int xen_cpu_intr_count(struct cpu_info *ci);

u_int
xen_cpu_intr_count(struct cpu_info *ci)
{

	KASSERT(ci->ci_nintrhand >= 0);

	return ci->ci_nintrhand;
}

static const char *
xen_intr_string(int port, char *buf, size_t len, struct pic *pic)
{
	KASSERT(pic->pic_type == PIC_XEN);

	KASSERT(port >= 0);
	KASSERT(port < NR_EVENT_CHANNELS);

	snprintf(buf, len, "%s channel %d", pic->pic_name, port);

	return buf;
}

static const char *
legacy_intr_string(int ih, char *buf, size_t len, struct pic *pic)
{
	int legacy_irq;

	KASSERT(pic->pic_type == PIC_I8259);
#if NLAPIC > 0
	KASSERT(APIC_IRQ_ISLEGACY(ih));

	legacy_irq = APIC_IRQ_LEGACY_IRQ(ih);
#else
	legacy_irq = ih;
#endif
	KASSERT(legacy_irq >= 0 && legacy_irq < 16);

	snprintf(buf, len, "%s pin %d", pic->pic_name, legacy_irq);

	return buf;
}

const char * xintr_string(intr_handle_t ih, char *buf, size_t len);

const char *
xintr_string(intr_handle_t ih, char *buf, size_t len)
{
#if NIOAPIC > 0
	struct ioapic_softc *pic;
#endif

	if (ih == 0)
		panic("%s: bogus handle 0x%" PRIx64, __func__, ih);

#if NIOAPIC > 0
	if (ih & APIC_INT_VIA_APIC) {
		pic = ioapic_find(APIC_IRQ_APIC(ih));
		if (pic != NULL) {
			snprintf(buf, len, "%s pin %d",
			    device_xname(pic->sc_dev), APIC_IRQ_PIN(ih));
		} else {
			snprintf(buf, len,
			    "apic %d int %d (irq %d)",
			    APIC_IRQ_APIC(ih),
			    APIC_IRQ_PIN(ih),
			    APIC_IRQ_LEGACY_IRQ(ih));
		}
	} else
		snprintf(buf, len, "irq %d", APIC_IRQ_LEGACY_IRQ(ih));

#elif NLAPIC > 0
	snprintf(buf, len, "irq %d", APIC_IRQ_LEGACY_IRQ(ih));
#else
	snprintf(buf, len, "irq %d", (int) ih);
#endif
	return buf;

}

/*
 * Create an interrupt id such as "ioapic0 pin 9". This interrupt id is used
 * by MI code and intrctl(8).
 */
const char * xen_intr_create_intrid(int legacy_irq, struct pic *pic,
    int pin, char *buf, size_t len);

const char *
xen_intr_create_intrid(int legacy_irq, struct pic *pic, int pin, char *buf, size_t len)
{
	int ih = 0;

#if NPCI > 0
#if defined(__HAVE_PCI_MSI_MSIX)
	if ((pic->pic_type == PIC_MSI) || (pic->pic_type == PIC_MSIX)) {
		uint64_t pih;
		int dev, vec;

		dev = msipic_get_devid(pic);
		vec = pin;
		pih = __SHIFTIN((uint64_t)dev, MSI_INT_DEV_MASK)
			| __SHIFTIN((uint64_t)vec, MSI_INT_VEC_MASK)
			| APIC_INT_VIA_MSI;
		if (pic->pic_type == PIC_MSI)
			MSI_INT_MAKE_MSI(pih);
		else if (pic->pic_type == PIC_MSIX)
			MSI_INT_MAKE_MSIX(pih);

		return x86_pci_msi_string(NULL, pih, buf, len);
	}
#endif /* __HAVE_PCI_MSI_MSIX */
#endif

	if (pic->pic_type == PIC_XEN) {
		ih = pin;	/* Port == pin */
		return xen_intr_string(pin, buf, len, pic);
	}

	/*
	 * If the device is pci, "legacy_irq" is alway -1. Least 8 bit of "ih"
	 * is only used in intr_string() to show the irq number.
	 * If the device is "legacy"(such as floppy), it should not use
	 * intr_string().
	 */
	if (pic->pic_type == PIC_I8259) {
		ih = legacy_irq;
		return legacy_intr_string(ih, buf, len, pic);
	}

#if NIOAPIC > 0 || NACPICA > 0
	ih = ((pic->pic_apicid << APIC_INT_APIC_SHIFT) & APIC_INT_APIC_MASK)
	    | ((pin << APIC_INT_PIN_SHIFT) & APIC_INT_PIN_MASK);
	if (pic->pic_type == PIC_IOAPIC) {
		ih |= APIC_INT_VIA_APIC;
	}
	ih |= pin;
	return intr_string(ih, buf, len);
#endif

	return NULL; /* No pic found! */
}

#if !defined(XENPVHVM)
__strong_alias(spllower, xen_spllower);
__strong_alias(x86_read_psl, xen_read_psl);
__strong_alias(x86_write_psl, xen_write_psl);

__strong_alias(intr_string, xintr_string);
__strong_alias(intr_create_intrid, xen_intr_create_intrid);
__strong_alias(intr_establish, xen_intr_establish);
__strong_alias(intr_establish_xname, xen_intr_establish_xname);
__strong_alias(intr_disestablish, xen_intr_disestablish);
__strong_alias(cpu_intr_redistribute, xen_cpu_intr_redistribute);
__strong_alias(cpu_intr_count, xen_cpu_intr_count);
__strong_alias(cpu_intr_init, xen_cpu_intr_init);
#endif /* !XENPVHVM */
