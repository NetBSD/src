/*	$NetBSD: umass_component.c,v 1.1.12.1 2016/03/19 11:30:35 skrll Exp $	*/

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
