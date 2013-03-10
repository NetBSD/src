/*	$NetBSD: component.c,v 1.2 2013/03/10 21:01:19 christos Exp $	*/

/*
 * Public Domain.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: component.c,v 1.2 2013/03/10 21:01:19 christos Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/stat.h>

#include "rump_private.h"
#include "rump_vfs_private.h"

extern const struct cdevsw npf_cdevsw;

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	devmajor_t bmajor = NODEVMAJOR, cmajor = NODEVMAJOR;
	int error;

	error = devsw_attach("npf", NULL, &bmajor, &npf_cdevsw, &cmajor);
	if (error) {
		panic("npf attach failed: %d", error);
	}

	error = rump_vfs_makeonedevnode(S_IFCHR, "/dev/npf", cmajor, 0);
	if (error) {
		panic("npf device node creation failed: %d", error);
	}
	devsw_detach(NULL, &npf_cdevsw);
}
