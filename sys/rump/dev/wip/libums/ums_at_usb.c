/*	$NetBSD: ums_at_usb.c,v 1.4.2.2 2010/03/11 15:04:35 yamt Exp $	*/

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/mount.h>

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

	FLAWLESSCALL(config_cfdata_attach(cfdata_ums, 0));

	FLAWLESSCALL(config_cfdriver_attach(&uhidev_cd));
	FLAWLESSCALL(config_cfattach_attach("uhidev", &uhidev_ca));

	FLAWLESSCALL(config_cfdriver_attach(&ums_cd));
	FLAWLESSCALL(config_cfattach_attach("ums", &ums_ca));
}
