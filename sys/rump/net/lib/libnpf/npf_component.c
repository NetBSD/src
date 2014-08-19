/*	$NetBSD: npf_component.c,v 1.1.10.2 2014/08/20 00:04:43 tls Exp $	*/

/*
 * Public Domain.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_component.c,v 1.1.10.2 2014/08/20 00:04:43 tls Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/stat.h>

#include "rump_private.h"
#include "rump_vfs_private.h"

extern const struct cdevsw npf_cdevsw;

RUMP_COMPONENT(RUMP_COMPONENT_NET)
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
