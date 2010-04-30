/*	$NetBSD: ubt_at_usb.c,v 1.4.2.2 2010/04/30 14:44:24 uebayasi Exp $	*/

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
