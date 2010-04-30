/*	$NetBSD: usb_at_ugenhc.c,v 1.3.2.2 2010/04/30 14:44:25 uebayasi Exp $	*/

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

	config_init_component(cfdriver_ioconf_usb,
	    cfattach_ioconf_usb, cfdata_ioconf_usb);
}
