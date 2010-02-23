/*	$NetBSD: rmixl_cpucorevar.h,v 1.1.2.2 2010/02/23 20:33:48 matt Exp $	q*/

#ifndef _ARCH_MIPS_RMI_RMIXL_CPUCOREVAR_H_
#define _ARCH_MIPS_RMI_RMIXL_CPUCOREVAR_H_

struct cpucore_softc {
	device_t	sc_dev;
	bool		sc_attached;
	u_int		sc_core;
#ifdef MULTIPROCESSOR
	struct pmap_tlb_info *sc_tlbinfo;
	struct pmap_tlb_info sc_tlbinfo0;
#endif
};

struct cpucore_attach_args {
	const char     *ca_name;
	int		ca_core;
	int		ca_thread;
};

#endif	/* _ARCH_MIPS_RMI_RMIXL_CPUCOREVAR_H_ */
