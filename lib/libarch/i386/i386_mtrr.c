/*	$NetBSD: i386_mtrr.c,v 1.1 2001/09/10 10:20:32 fvdl Exp $	*/

#include <sys/cdefs.h>
#include <sys/types.h>

#include <machine/sysarch.h>

int
i386_get_mtrr(struct mtrr *mtrrp, int *n)
{
	struct i386_get_mtrr_args a;

	a.mtrrp = mtrrp;
	a.n = n;
	return sysarch(I386_GET_MTRR, (void *)&a);
}

int
i386_set_mtrr(struct mtrr *mtrrp, int *n)
{
	struct i386_set_mtrr_args a;

	a.mtrrp = mtrrp;
	a.n = n;
	return sysarch(I386_SET_MTRR, (void *)&a);
}
