/*	$NetBSD: umass_component.c,v 1.2.18.1 2019/06/10 22:09:51 christos Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: umass_component.c,v 1.2.18.1 2019/06/10 22:09:51 christos Exp $");

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
