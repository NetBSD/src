/*	$NetBSD: ugenhc_at_mainbus.c,v 1.1.2.2 2010/03/11 15:04:34 yamt Exp $	*/

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

	FLAWLESSCALL(config_cfdata_attach(cfdata_ugenhc, 0));

	FLAWLESSCALL(config_cfdriver_attach(&ugenhc_cd));
	FLAWLESSCALL(config_cfattach_attach("ugenhc", &ugenhc_ca));
}
