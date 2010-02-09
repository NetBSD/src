/*	$NetBSD: sd_at_scsibus_at_umass.c,v 1.8 2010/02/09 19:02:19 pooka Exp $	*/

#include <sys/param.h>
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

void
rump_device_configuration(void)
{
	extern struct cfattach rumpusbhc_ca;
	extern struct cfattach usb_ca, uhub_ca, uroothub_ca, umass_ca;
	extern struct cfattach scsibus_ca, atapibus_ca, sd_ca, cd_ca;
	extern struct bdevsw sd_bdevsw;
	extern struct cdevsw sd_cdevsw;
	devmajor_t bmaj, cmaj;

	FLAWLESSCALL(config_cfdata_attach(cfdata_umass, 0));

	FLAWLESSCALL(config_cfdriver_attach(&mainbus_cd));
	FLAWLESSCALL(config_cfattach_attach("mainbus", &mainbus_ca));

	FLAWLESSCALL(config_cfdriver_attach(&rumpusbhc_cd));
	FLAWLESSCALL(config_cfattach_attach("rumpusbhc", &rumpusbhc_ca));

	FLAWLESSCALL(config_cfdriver_attach(&usb_cd));
	FLAWLESSCALL(config_cfattach_attach("usb", &usb_ca));

	FLAWLESSCALL(config_cfdriver_attach(&uhub_cd));
	FLAWLESSCALL(config_cfattach_attach("uhub", &uhub_ca));

	FLAWLESSCALL(config_cfdriver_attach(&umass_cd));
	FLAWLESSCALL(config_cfattach_attach("umass", &umass_ca));

	FLAWLESSCALL(config_cfdriver_attach(&scsibus_cd));
	FLAWLESSCALL(config_cfattach_attach("scsibus", &scsibus_ca));

	FLAWLESSCALL(config_cfdriver_attach(&atapibus_cd));
	FLAWLESSCALL(config_cfattach_attach("atapibus", &atapibus_ca));

	FLAWLESSCALL(config_cfdriver_attach(&sd_cd));
	FLAWLESSCALL(config_cfattach_attach("sd", &sd_ca));

	FLAWLESSCALL(config_cfdriver_attach(&cd_cd));
	FLAWLESSCALL(config_cfattach_attach("cd", &cd_ca));

	FLAWLESSCALL(config_cfattach_attach("uhub", &uroothub_ca));

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("sd", &sd_bdevsw, &bmaj, &sd_cdevsw, &cmaj));

	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFBLK, "/dev/sd0", 'a',
	    bmaj, 0, 8));
	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/rsd0", 'a',
	    cmaj, 0, 8));
}
