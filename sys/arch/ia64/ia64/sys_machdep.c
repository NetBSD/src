
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: sys_machdep.c,v 1.1 2006/04/07 14:21:18 cherry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

int
sys_sysarch(struct lwp *l, void *v, register_t *retval)
{
	return (EINVAL);
}
