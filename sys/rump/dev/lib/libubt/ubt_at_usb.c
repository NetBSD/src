/*	$NetBSD: ubt_at_usb.c,v 1.4.4.2 2010/05/30 05:18:04 rmind Exp $	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include "ioconf.c"

#include "rump_private.h"
#include "rump_dev_private.h"

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{

	config_init_component(cfdriver_ioconf_ubt,
	    cfattach_ioconf_ubt, cfdata_ioconf_ubt);
}
