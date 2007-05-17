/*	$NetBSD: atomic.h,v 1.4.2.2 2007/05/17 00:43:55 jdc Exp $	*/

#if defined(__i386__) || defined(__x86_64__)
/* Bind shares those for now, although it provides both of them */
#include "lib/isc/x86_32/include/isc/atomic.h"
#elif defined(__sparc__) && defined(_LP64)
#include "lib/isc/sparc64/include/isc/atomic.h"
#elif defined(__alpha__)
#include "lib/isc/alpha/include/isc/atomic.h"
#elif defined(__powerpc__) && defined(notyet)
#include "lib/isc/powerpc/include/isc/atomic.h"
#elif defined(__mips__) && !defined(_MIPS_ARCH_MIPS1)
#include "lib/isc/mips/include/isc/atomic.h"
#elif defined(__ia64__)
#include "lib/isc/ia64/include/isc/atomic.h"
#else
#include "lib/isc/noatomic/include/isc/atomic.h"
#endif
