#include <sys/cdefs.h>

__RCSID("$NetBSD: mdtls.c,v 1.1 2011/03/14 08:20:15 skrll Exp $");

#include <sys/tls.h>
#include "rtld.h"

__dso_public void *__tls_get_addr(int[2]);

void *
__tls_get_addr(int idx[2])
{
	void *p;

        __asm volatile("mfctl\t27 /* CR_TLS */, %0" : "=r" (p));

	return _rtld_tls_get_addr(p, idx[0], idx[1]);
}
