/*	$NetBSD: x86_64_mtrr.c,v 1.1 2002/06/18 08:29:36 fvdl Exp $	*/

#include <sys/cdefs.h>
#include <sys/types.h>

#include <machine/sysarch.h>

int
x86_64_get_mtrr(struct mtrr *mtrrp, int *n)
{
	struct x86_64_get_mtrr_args a;

	a.mtrrp = mtrrp;
	a.n = n;
	return sysarch(X86_64_GET_MTRR, (void *)&a);
}

int
x86_64_set_mtrr(struct mtrr *mtrrp, int *n)
{
	struct x86_64_set_mtrr_args a;

	a.mtrrp = mtrrp;
	a.n = n;
	return sysarch(X86_64_SET_MTRR, (void *)&a);
}
