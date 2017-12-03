/*	 $NetBSD: i82093var.h,v 1.4.18.1 2017/12/03 11:36:51 jdolecek Exp $ */

#ifndef _XEN_I82093VAR_H_
#define _XEN_I82093VAR_H_

#include "opt_xen.h"
#define _IOAPIC_CUSTOM_RW
#include <x86/i82093var.h>
#include <hypervisor.h>

static inline  uint32_t
ioapic_read_ul(struct ioapic_softc *sc, int regid)
{
	physdev_op_t op;
	int ret;

        op.cmd = PHYSDEVOP_APIC_READ;
	op.u.apic_op.apic_physbase = sc->sc_pa;
	op.u.apic_op.reg = regid;
	ret = HYPERVISOR_physdev_op(&op);
	if (ret) {
		printf("PHYSDEVOP_APIC_READ ret %d\n", ret);
		panic("PHYSDEVOP_APIC_READ");
	}
	return op.u.apic_op.value;      
}

static inline void
ioapic_write_ul(struct ioapic_softc *sc, int regid, uint32_t val)
{
	physdev_op_t op;
	int ret;

        op.cmd = PHYSDEVOP_APIC_WRITE;
	op.u.apic_op.apic_physbase = sc->sc_pa;
	op.u.apic_op.reg = regid;
	op.u.apic_op.value = val;
	ret = HYPERVISOR_physdev_op(&op);
	if (ret)
		printf("PHYSDEVOP_APIC_WRITE ret %d\n", ret);
}

#endif /* !_XEN_I82093VAR_H_ */
