/*	$NetBSD: scsipi_component.c,v 1.1.10.3 2017/12/03 11:39:09 jdolecek Exp $	*/

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/stat.h>

#include "ioconf.c"

#include <rump-sys/kern.h>
#include <rump-sys/vfs.h>

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	extern struct bdevsw sd_bdevsw, cd_bdevsw;
	extern struct cdevsw sd_cdevsw, cd_cdevsw;
	devmajor_t bmaj, cmaj;

	config_init_component(cfdriver_ioconf_scsipi,
	    cfattach_ioconf_scsipi, cfdata_ioconf_scsipi);

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
