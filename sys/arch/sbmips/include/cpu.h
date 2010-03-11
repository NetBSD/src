/* $NetBSD: cpu.h,v 1.1.142.1 2010/03/11 08:20:59 matt Exp $ */

#ifndef _SBMIPS_CPU_H_
#define _SBMIPS_CPU_H_

#include <mips/cpu.h>

#ifndef _LOCORE
struct cpu_softc {
	device_t cpu_dev;
	struct cpu_info *cpu_ci;
	vaddr_t cpu_imr_base;
	uint64_t cpu_imr_all;
	struct evcnt cpu_intr_evcnts[64];
};
#endif /* _LOCORE */

#endif /* !_SBMIPS_CPU_H_ */
