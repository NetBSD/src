/*	$NetBSD: cdefs.h,v 1.1.4.1 2001/10/01 12:38:44 fvdl Exp $	*/

/* Windows CE architecture */

#define __BEGIN_MACRO	do {
#define __END_MACRO	} while (/*CONSTCOND*/0)

#define NAMESPACE_BEGIN(x)	namespace x {
#define NAMESPACE_END		}
#define USING_NAMESPACE(x)	using namespace x;

#include <sys/cdefs.h>
