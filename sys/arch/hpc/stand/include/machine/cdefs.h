/*	$NetBSD: cdefs.h,v 1.1.6.1 2002/01/10 19:43:34 thorpej Exp $	*/

/* Windows CE architecture */

#define __BEGIN_MACRO	do {
#define __END_MACRO	} while (/*CONSTCOND*/0)

#define NAMESPACE_BEGIN(x)	namespace x {
#define NAMESPACE_END		}
#define USING_NAMESPACE(x)	using namespace x;

#include <sys/cdefs.h>
