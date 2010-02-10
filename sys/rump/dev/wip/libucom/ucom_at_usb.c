/*	$NetBSD: ucom_at_usb.c,v 1.4 2010/02/10 02:26:23 pooka Exp $	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/stat.h>

#include "ioconf.c"

#include "rump_dev_private.h"
#include "rump_vfs_private.h"

#define FLAWLESSCALL(call)						\
do {									\
	int att_error;							\
	if ((att_error = call) != 0)					\
		panic("\"%s\" failed", #call);				\
} while (/*CONSTCOND*/0)

void tty_init(void);

void
rump_device_configuration(void)
{
	extern struct cdevsw ucom_cdevsw;
	devmajor_t cmaj, bmaj;

	FLAWLESSCALL(config_cfdata_attach(cfdata_ucom, 0));

	FLAWLESSCALL(config_cfdriver_attach(&mainbus_cd));
	FLAWLESSCALL(config_cfattach_attach("mainbus", &mainbus_ca));

	FLAWLESSCALL(config_cfdriver_attach(&ugenhc_cd));
	FLAWLESSCALL(config_cfattach_attach("ugenhc", &ugenhc_ca));

	FLAWLESSCALL(config_cfdriver_attach(&usb_cd));
	FLAWLESSCALL(config_cfattach_attach("usb", &usb_ca));

	FLAWLESSCALL(config_cfdriver_attach(&uplcom_cd));
	FLAWLESSCALL(config_cfattach_attach("uplcom", &uplcom_ca));

	FLAWLESSCALL(config_cfdriver_attach(&ucom_cd));
	FLAWLESSCALL(config_cfattach_attach("ucom", &ucom_ca));

	FLAWLESSCALL(config_cfdriver_attach(&uhub_cd));
	FLAWLESSCALL(config_cfattach_attach("uhub", &uroothub_ca));

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("ucom", NULL, &bmaj, &ucom_cdevsw, &cmaj));

	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/ttyU", '0',
	    cmaj, 0, 1));
	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/dtyU", '0',
	    cmaj, 0x80000, 1));

	tty_init();
}
