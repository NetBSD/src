/*	$NetBSD: lwp_private.h,v 1.1 2024/11/30 01:04:09 christos Exp $	*/

#ifndef _HPPA_LWP_PRIVATE_H_
#define	_HPPA_LWP_PRIVATE_H_

#include <sys/cdefs.h>
#include <sys/tls.h>

__BEGIN_DECLS
static __inline void *
__lwp_getprivate_fast(void)
{
	register void *__tmp;

	__asm volatile("mfctl\t27 /* CR_TLS */, %0" : "=r" (__tmp));

	return __tmp;
}
__END_DECLS

#endif /* _HPPA_LWP_PRIVATE_H_ */
