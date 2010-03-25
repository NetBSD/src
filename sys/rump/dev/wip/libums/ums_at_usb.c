/*	$NetBSD: ums_at_usb.c,v 1.5 2010/03/25 19:54:08 pooka Exp $	*/

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/mount.h>

#include "ioconf.c"

#include "rump_private.h"
#include "rump_dev_private.h"
#include "rump_vfs_private.h"

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{

	config_init_component(cfdriver_comp_ums,
	    cfattach_comp_ums, cfdata_ums);
}
