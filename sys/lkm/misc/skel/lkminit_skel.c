/* $NetBSD: lkminit_skel.c,v 1.2.4.2 2007/02/26 09:11:26 yamt Exp $ */

/* This code is in the public domain. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lkminit_skel.c,v 1.2.4.2 2007/02/26 09:11:26 yamt Exp $");

#include <sys/param.h>
#include <sys/lkm.h>

int skel_lkmentry(struct lkm_table *lkmtp, int, int);
static int skelmod_handle(struct lkm_table *, int);

/* The module name, as appears in 'modstat' (and used in 'modunload'). */
MOD_MISC("skelmod");

static int
skelmod_handle(struct lkm_table *lkmtp, int cmd)
{
	int error = 0;

	switch (cmd) {
	case LKM_E_LOAD:
		if (lkmexists(lkmtp))
			return (EEXIST);

		printf("hello world\n");

		break;

	case LKM_E_UNLOAD:
		printf("goodbye world\n");
		break;

	default:
		error = EINVAL;
		break;
	}

	return (error);
}

int
skel_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{
	DISPATCH(lkmtp, cmd, ver, skelmod_handle, skelmod_handle, lkm_nofunc)
}
