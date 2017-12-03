/* $NetBSD: cpuvar.h,v 1.1.1.1.6.2 2017/12/03 11:36:10 jdolecek Exp $ */

#ifndef _SBMIPS_CPUVAR_H_
#define _SBMIPS_CPUVAR_H_

#include <mips/cpu.h>

#ifndef _LOCORE
struct cpu_softc {
	device_t sb1cpu_dev;
	struct cpu_info *sb1cpu_ci;
	vaddr_t sb1cpu_imr_base;
	uint64_t sb1cpu_imr_all;
	struct evcnt sb1cpu_intr_evcnts[64];
	struct evcnt sb1cpu_spurious_intrs[_IPL_N];
	struct evcnt sb1cpu_intrs[_IPL_N];
	struct evcnt sb1cpu_int5;
};
#endif /* _LOCORE */

#endif /* !_SBMIPS_CPUVAR_H_ */
