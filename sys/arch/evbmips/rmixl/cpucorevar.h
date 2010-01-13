/*	$NetBSD: cpucorevar.h,v 1.1.2.1 2010/01/13 09:40:35 cliff Exp $	*/

#ifndef _EVBMIPS_RMIXL_CPUCOREVAR_H_
#define _EVBMIPS_RMIXL_CPUCOREVAR_H_

struct cpucore_attach_args {
	const char     *ca_name;
	int		ca_thread;
};

#endif	/* _EVBMIPS_RMIXL_CPUCOREVAR_H_ */
