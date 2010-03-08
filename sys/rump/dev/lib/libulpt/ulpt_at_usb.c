/*	$NetBSD: ulpt_at_usb.c,v 1.3 2010/03/08 10:30:17 pooka Exp $	*/

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include "ioconf.c"

#include "rump_private.h"
#include "rump_dev_private.h"
#include "rump_vfs_private.h"

#define FLAWLESSCALL(call)						\
do {									\
	int att_error;							\
	if ((att_error = call) != 0)					\
		panic("\"%s\" failed", #call);				\
} while (/*CONSTCOND*/0)

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	extern struct cdevsw ulpt_cdevsw;
	devmajor_t bmaj, cmaj;

	FLAWLESSCALL(config_cfdata_attach(cfdata_ulpt, 0));

	FLAWLESSCALL(config_cfdriver_attach(&ulpt_cd));
	FLAWLESSCALL(config_cfattach_attach("ulpt", &ulpt_ca));

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("ulpt", NULL, &bmaj, &ulpt_cdevsw, &cmaj));

	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/ulpt", '0',
	    cmaj, 0, 1));
}
