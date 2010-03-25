/*	$NetBSD: ubt_at_usb.c,v 1.3 2010/03/25 19:54:08 pooka Exp $	*/

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

	config_init_component(cfdriver_comp_ubt, cfattach_comp_ubt, cfdata_ubt);
}
