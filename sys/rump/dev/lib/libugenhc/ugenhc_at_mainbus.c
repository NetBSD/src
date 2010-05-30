/*	$NetBSD: ugenhc_at_mainbus.c,v 1.1.4.1 2010/05/30 05:18:04 rmind Exp $	*/

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

	config_init_component(cfdriver_ioconf_ugenhc,
	    cfattach_ioconf_ugenhc, cfdata_ioconf_ugenhc);
}
