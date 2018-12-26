/*	$NetBSD: ulpt_at_usb.c,v 1.6.16.1 2018/12/26 14:02:06 pgoyette Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ulpt_at_usb.c,v 1.6.16.1 2018/12/26 14:02:06 pgoyette Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include "ioconf.c"

#include <rump-sys/kern.h>
#include <rump-sys/vfs.h>

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
