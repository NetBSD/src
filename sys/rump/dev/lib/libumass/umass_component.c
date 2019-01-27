/*	$NetBSD: umass_component.c,v 1.5 2019/01/27 09:19:37 rin Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: umass_component.c,v 1.5 2019/01/27 09:19:37 rin Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/stat.h>

#include "ioconf.c"

#include <rump-sys/kern.h>

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{

	config_init_component(cfdriver_ioconf_umass,
	    cfattach_ioconf_umass, cfdata_ioconf_umass);
}
