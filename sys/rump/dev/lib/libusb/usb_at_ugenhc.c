/*	$NetBSD: usb_at_ugenhc.c,v 1.2 2010/03/25 19:54:08 pooka Exp $	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/stat.h>

#include "ioconf.c"

#include "rump_private.h"
#include "rump_dev_private.h"

void tty_init(void);

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{

	config_init_component(cfdriver_comp_usb, cfattach_comp_usb, cfdata_usb);
}
