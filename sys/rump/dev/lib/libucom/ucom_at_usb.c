/*	$NetBSD: ucom_at_usb.c,v 1.5 2010/03/08 10:24:37 pooka Exp $	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/stat.h>

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

void tty_init(void);

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	extern struct cdevsw ucom_cdevsw;
	devmajor_t cmaj, bmaj;

	FLAWLESSCALL(config_cfdata_attach(cfdata_ucom, 0));

	FLAWLESSCALL(config_cfdriver_attach(&uplcom_cd));
	FLAWLESSCALL(config_cfattach_attach("uplcom", &uplcom_ca));

	FLAWLESSCALL(config_cfdriver_attach(&u3ginit_cd));
	FLAWLESSCALL(config_cfattach_attach("u3ginit", &u3ginit_ca));

	FLAWLESSCALL(config_cfdriver_attach(&u3g_cd));
	FLAWLESSCALL(config_cfattach_attach("u3g", &u3g_ca));

	FLAWLESSCALL(config_cfdriver_attach(&ucom_cd));
	FLAWLESSCALL(config_cfattach_attach("ucom", &ucom_ca));

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("ucom", NULL, &bmaj, &ucom_cdevsw, &cmaj));

	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/ttyU", '0',
	    cmaj, 0, 2));
	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/dtyU", '0',
	    cmaj, 0x80000, 2));

	tty_init();
}
