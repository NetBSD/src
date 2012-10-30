/*	$NetBSD: checkrev.c,v 1.1.1.1.2.3 2012/10/30 18:55:04 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: checkrev.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $
 */

#include <sys/ioctl.h>
#include <fcntl.h>

#include "ipf.h"
#include "netinet/ipl.h"

int checkrev(ipfname)
	char *ipfname;
{
	static int vfd = -1;
	struct friostat fio;
	ipfobj_t obj;

	bzero((caddr_t)&obj, sizeof(obj));
	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_size = sizeof(fio);
	obj.ipfo_ptr = (void *)&fio;
	obj.ipfo_type = IPFOBJ_IPFSTAT;

	if ((vfd == -1) && ((vfd = open(ipfname, O_RDONLY)) == -1)) {
		perror("open device");
		return -1;
	}

	if (ioctl(vfd, SIOCGETFS, &obj)) {
		ipferror(vfd, "ioctl(SIOCGETFS)");
		close(vfd);
		vfd = -1;
		return -1;
	}

	if (strncmp(IPL_VERSION, fio.f_version, sizeof(fio.f_version))) {
		return -1;
	}
	return 0;
}
