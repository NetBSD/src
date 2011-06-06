/* $NetBSD: x86_stub.c,v 1.1.8.2 2011/06/06 09:07:10 jruoho Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: x86_stub.c,v 1.1.8.2 2011/06/06 09:07:10 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kgdb.h>

int x86_nullop(void);
void x86_voidop(void);

void
x86_voidop(void)
{
}

int
x86_nullop(void)
{
	return 0;
}

__weak_alias(kdb_trap, x86_nullop);
__weak_alias(kgdb_disconnected, x86_nullop);
__weak_alias(kgdb_trap, x86_nullop);
__weak_alias(mca_nmi, x86_voidop);
__weak_alias(x86_nmi, x86_voidop);
