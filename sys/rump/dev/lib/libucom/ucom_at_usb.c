/*	$NetBSD: ucom_at_usb.c,v 1.7.38.1 2016/03/19 11:30:35 skrll Exp $	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/stat.h>

#include "ioconf.c"

#include <rump-sys/kern.h>
#include <rump-sys/vfs.h>

void tty_init(void);

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	extern struct cdevsw ucom_cdevsw;
	devmajor_t cmaj, bmaj;

	config_init_component(cfdriver_ioconf_ucom,
	    cfattach_ioconf_ucom, cfdata_ioconf_ucom);

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("ucom", NULL, &bmaj, &ucom_cdevsw, &cmaj));

	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/ttyU", '0',
	    cmaj, 0, 2));
	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/dtyU", '0',
	    cmaj, 0x80000, 2));

	tty_init();
}
