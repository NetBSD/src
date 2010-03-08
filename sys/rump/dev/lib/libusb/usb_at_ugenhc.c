/*	$NetBSD: usb_at_ugenhc.c,v 1.1 2010/03/08 10:24:37 pooka Exp $	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/stat.h>

#include "ioconf.c"

#include "rump_private.h"
#include "rump_dev_private.h"

#define FLAWLESSCALL(call)						\
do {									\
	int att_error;							\
	if ((att_error = call) != 0)					\
		panic("\"%s\" failed", #call);				\
} while (/*CONSTCOND*/0)

void tty_init(void);

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{

	FLAWLESSCALL(config_cfdata_attach(cfdata_usb, 0));

	FLAWLESSCALL(config_cfdriver_attach(&usb_cd));
	FLAWLESSCALL(config_cfattach_attach("usb", &usb_ca));

	FLAWLESSCALL(config_cfdriver_attach(&uhub_cd));
	FLAWLESSCALL(config_cfattach_attach("uhub", &uroothub_ca));
}
