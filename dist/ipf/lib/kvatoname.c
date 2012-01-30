/*	$NetBSD: kvatoname.c,v 1.1.1.4 2012/01/30 16:03:22 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: kvatoname.c,v 1.6.2.1 2012/01/26 05:29:16 darrenr Exp
 */

#include "ipf.h"

#include <fcntl.h>
#include <sys/ioctl.h>

char *kvatoname(func, iocfunc)
	ipfunc_t func;
	ioctlfunc_t iocfunc;
{
	static char funcname[40];
	ipfunc_resolve_t res;
	int fd;

	res.ipfu_addr = func;
	res.ipfu_name[0] = '\0';
	fd = -1;

	if ((opts & OPT_DONTOPEN) == 0) {
		fd = open(IPL_NAME, O_RDONLY);
		if (fd == -1)
			return NULL;
	}
	(void) (*iocfunc)(fd, SIOCFUNCL, &res);
	if (fd >= 0)
		close(fd);
	strncpy(funcname, res.ipfu_name, sizeof(funcname));
	funcname[sizeof(funcname) - 1] = '\0';
	return funcname;
}
