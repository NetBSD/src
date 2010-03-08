/*	$NetBSD: sd_at_scsibus_at_umass.c,v 1.3 2010/03/08 10:24:37 pooka Exp $	*/

#include <sys/param.h>
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

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	extern struct cfattach umass_ca;
	extern struct cfattach scsibus_ca, atapibus_ca, sd_ca, cd_ca;
	extern struct bdevsw sd_bdevsw, cd_bdevsw;
	extern struct cdevsw sd_cdevsw, cd_cdevsw;
	devmajor_t bmaj, cmaj;

	FLAWLESSCALL(config_cfdata_attach(cfdata_umass, 0));

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

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("sd", &sd_bdevsw, &bmaj, &sd_cdevsw, &cmaj));

	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFBLK, "/dev/sd0", 'a',
	    bmaj, 0, 8));
	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/rsd0", 'a',
	    cmaj, 0, 8));

	bmaj = cmaj = -1;
	FLAWLESSCALL(devsw_attach("cd", &cd_bdevsw, &bmaj, &cd_cdevsw, &cmaj));

	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFBLK, "/dev/cd0", 'a',
	    bmaj, 0, 8));
	FLAWLESSCALL(rump_vfs_makedevnodes(S_IFCHR, "/dev/rcd0", 'a',
	    cmaj, 0, 8));
}
