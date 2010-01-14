/*	$NetBSD: cpucorevar.h,v 1.1.2.2 2010/01/14 00:40:35 matt Exp $	*/

#ifndef _EVBMIPS_RMIXL_CPUCOREVAR_H_
#define _EVBMIPS_RMIXL_CPUCOREVAR_H_

struct cpucore_softc {
	device_t	sc_dev;
	bool		sc_attached;
	u_int		sc_core;
};

struct cpucore_attach_args {
	const char     *ca_name;
	int		ca_core;
	int		ca_thread;
};

#endif	/* _EVBMIPS_RMIXL_CPUCOREVAR_H_ */
