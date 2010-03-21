/*	$NetBSD: rmixl_cpuvar.h,v 1.1.2.1 2010/03/21 21:20:10 cliff Exp $	*/

#ifndef _ARCH_MIPS_RMI_RMIXL_CPUVAR_H_
#define _ARCH_MIPS_RMI_RMIXL_CPUVAR_H_

struct rmixl_cpu_trampoline_args {
	uint64_t	ta_sp;
	uint64_t	ta_lwp;
	uint64_t	ta_cpuinfo;
};

struct rmixl_cpu_softc {
	device_t sc_dev;
	struct cpu_info *sc_ci;
	void *sc_ih_clk;
	void *sc_ih_fmn;
	void *sc_ih_ipi;
	struct evcnt sc_vec_evcnts[64];
};

#endif	/* _ARCH_MIPS_RMI_RMIXL_CPUVAR_H_ */
