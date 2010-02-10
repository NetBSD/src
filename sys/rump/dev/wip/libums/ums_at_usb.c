/*	$NetBSD: ums_at_usb.c,v 1.3 2010/02/10 02:26:24 pooka Exp $	*/

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/mount.h>

#include "ioconf.c"

#include "rump_dev_private.h"
#include "rump_vfs_private.h"

#define FLAWLESSCALL(call)						\
do {									\
	int att_error;							\
	if ((att_error = call) != 0)					\
		panic("\"%s\" failed", #call);				\
} while (/*CONSTCOND*/0)

void
rump_device_configuration(void)
{

	FLAWLESSCALL(config_cfdata_attach(cfdata_ums, 0));

	FLAWLESSCALL(config_cfdriver_attach(&mainbus_cd));
	FLAWLESSCALL(config_cfattach_attach("mainbus", &mainbus_ca));

	FLAWLESSCALL(config_cfdriver_attach(&ugenhc_cd));
	FLAWLESSCALL(config_cfattach_attach("ugenhc", &ugenhc_ca));

	FLAWLESSCALL(config_cfdriver_attach(&usb_cd));
	FLAWLESSCALL(config_cfattach_attach("usb", &usb_ca));

	FLAWLESSCALL(config_cfdriver_attach(&uhidev_cd));
	FLAWLESSCALL(config_cfattach_attach("uhidev", &uhidev_ca));

	FLAWLESSCALL(config_cfdriver_attach(&ums_cd));
	FLAWLESSCALL(config_cfattach_attach("ums", &ums_ca));

	FLAWLESSCALL(config_cfdriver_attach(&uhub_cd));
	FLAWLESSCALL(config_cfattach_attach("uhub", &uroothub_ca));
}
