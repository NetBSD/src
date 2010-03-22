/*	$NetBSD: ubt_at_usb.c,v 1.1 2010/03/22 12:14:51 pooka Exp $	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include "ioconf.c"

#include "rump_private.h"
#include "rump_dev_private.h"

#define FLAWLESSCALL(call)						\
do {									\
	int att_error;							\
	if ((att_error = call) != 0)					\
		panic("\"%s\" failed", #call);				\
} while (/*CONSTCOND*/0)

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{

	FLAWLESSCALL(config_cfdata_attach(cfdata_ubt, 0));

	FLAWLESSCALL(config_cfdriver_attach(&ubt_cd));
	FLAWLESSCALL(config_cfattach_attach("ubt", &ubt_ca));
}
