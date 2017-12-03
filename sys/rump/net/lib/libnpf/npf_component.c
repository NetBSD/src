/*	$NetBSD: npf_component.c,v 1.1.10.3 2017/12/03 11:39:19 jdolecek Exp $	*/

/*
 * Public Domain.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_component.c,v 1.1.10.3 2017/12/03 11:39:19 jdolecek Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/stat.h>

#include <rump-sys/kern.h>
#include <rump-sys/vfs.h>

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
