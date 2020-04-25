/*	 $NetBSD: i82093var.h,v 1.7 2020/04/25 15:26:17 bouyer Exp $ */

#ifndef _XEN_I82093VAR_H_
#define _XEN_I82093VAR_H_

#include "opt_xen.h"
#define _IOAPIC_CUSTOM_RW
#include <x86/i82093var.h>
#include <hypervisor.h>

static inline  uint32_t
ioapic_read_ul(struct ioapic_softc *sc, int regid)
{
	struct physdev_apic apic_op;
	int ret;

	apic_op.apic_physbase = sc->sc_pa;
	apic_op.reg = regid;
	ret = HYPERVISOR_physdev_op(PHYSDEVOP_apic_read, &apic_op);
	if (ret) {
		printf("PHYSDEVOP_APIC_READ ret %d\n", ret);
		panic("PHYSDEVOP_APIC_READ");
	}
	return apic_op.value;      
}

static inline void
ioapic_write_ul(struct ioapic_softc *sc, int regid, uint32_t val)
{
	struct physdev_apic apic_op;
	int ret;

	apic_op.apic_physbase = sc->sc_pa;
	apic_op.reg = regid;
	apic_op.value = val;
	ret = HYPERVISOR_physdev_op(PHYSDEVOP_apic_write, &apic_op);
	if (ret)
		printf("PHYSDEVOP_APIC_WRITE ret %d\n", ret);
}

#endif /* !_XEN_I82093VAR_H_ */
