/*	$NetBSD: atomic.h,v 1.1 2007/01/27 21:36:13 christos Exp $	*/

#if defined(__i386__)
#include "lib/isc/x86_32/include/isc/atomic.h"
#elif defined(__x86_64__)
#include "lib/isc/x86_64/include/isc/atomic.h"
#elif defined(__sparc__) && defined(_LP64)
#include "lib/isc/sparc64/include/isc/atomic.h"
#elif defined(__alpha__)
#include "lib/isc/alpha/include/isc/atomic.h"
#elif defined(__powerpc__)
#include "lib/isc/powerpc/include/isc/atomic.h"
#elif defined(__mips__)
#include "lib/isc/mips/include/isc/atomic.h"
#elif defined(__mips__)
#include "lib/isc/mips/include/isc/atomic.h"
#elif defined(__ia64__)
#include "lib/isc/ia64/include/isc/atomic.h"
#else
#include "lib/isc/noatomic/include/isc/atomic.h"
#endif
