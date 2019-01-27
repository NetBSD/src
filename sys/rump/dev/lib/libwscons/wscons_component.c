/*	$NetBSD: wscons_component.c,v 1.5 2019/01/27 09:19:37 rin Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wscons_component.c,v 1.5 2019/01/27 09:19:37 rin Exp $");

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
	extern struct cdevsw wskbd_cdevsw, wsmouse_cdevsw;
	devmajor_t bmaj, cmaj;

	config_init_component(cfdriver_ioconf_wscons,
	    cfattach_ioconf_wscons, cfdata_ioconf_wscons);

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("wskbd", NULL, &bmaj, &wskbd_cdevsw, &cmaj));
	FLAWLESSCALL(rump_vfs_makeonedevnode(S_IFCHR, "/dev/wskbd", cmaj, 0));

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("wsmouse", NULL, &bmaj,
	    &wsmouse_cdevsw, &cmaj));
	FLAWLESSCALL(rump_vfs_makeonedevnode(S_IFCHR, "/dev/wsmouse", cmaj, 0));
}
