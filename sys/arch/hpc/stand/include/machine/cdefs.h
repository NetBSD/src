/*	$NetBSD: cdefs.h,v 1.2.20.1 2004/08/03 10:34:59 skrll Exp $	*/

/* Windows CE architecture */

#define __BEGIN_MACRO	do {
#define __END_MACRO	} while (/*CONSTCOND*/0)

#define NAMESPACE_BEGIN(x)	namespace x {
#define NAMESPACE_END		}
#define USING_NAMESPACE(x)	using namespace x;
