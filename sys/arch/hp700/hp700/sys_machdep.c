/*	$NetBSD: sys_machdep.c,v 1.4 2003/11/24 02:51:35 chs Exp $	*/

/*	$OpenBSD: sys_machdep.c,v 1.1 1998/12/29 18:06:48 mickey Exp $	*/


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_machdep.c,v 1.4 2003/11/24 02:51:35 chs Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

int
sys_sysarch(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sysarch_args /* {
		syscallarg(int) op;
		syscallarg(char *) parms;
	} */ *uap = v;
	int error = 0;

	switch (SCARG(uap, op)) {
	default:
		error = EINVAL;
		break;
	}
	return (error);
}
