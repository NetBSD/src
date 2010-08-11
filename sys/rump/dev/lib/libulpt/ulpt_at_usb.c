/*	$NetBSD: ulpt_at_usb.c,v 1.3.2.3 2010/08/11 22:55:02 yamt Exp $	*/

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include "ioconf.c"

#include "rump_private.h"
#include "rump_dev_private.h"
#include "rump_vfs_private.h"

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	extern struct cdevsw ulpt_cdevsw;
	devmajor_t bmaj, cmaj;

	config_init_component(cfdriver_ioconf_ulpt,
	    cfattach_ioconf_ulpt, cfdata_ioconf_ulpt);

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("ulpt", NULL, &bmaj, &ulpt_cdevsw, &cmaj));

	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/ulpt", '0',
	    cmaj, 0, 1));
}
