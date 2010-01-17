/*	$NetBSD: rmixl_cpunodevar.h,v 1.1.2.1 2010/01/17 00:11:11 cliff Exp $	*/

#ifndef _ARCH_MIPS_RMI_RMIXL_CPUNODEVAR_H_
#define _ARCH_MIPS_RMI_RMIXL_CPUNODEVAR_H_

struct cpunode_softc {
	device_t	sc_dev;
	bool		sc_attached;
	u_int		sc_node;
};

struct cpunode_attach_args {
	const char     *na_name;
	int		na_node;
	int		na_core;
};

#endif	/* _ARCH_MIPS_RMI_RMIXL_CPUNODEVAR_H_ */
