/*	$NetBSD: cdefs.h,v 1.3 2004/04/26 22:14:55 uwe Exp $	*/

/* Windows CE architecture */

#define __BEGIN_MACRO	do {
#define __END_MACRO	} while (/*CONSTCOND*/0)

#define NAMESPACE_BEGIN(x)	namespace x {
#define NAMESPACE_END		}
#define USING_NAMESPACE(x)	using namespace x;
