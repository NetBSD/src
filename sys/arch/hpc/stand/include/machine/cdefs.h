/*	$NetBSD: cdefs.h,v 1.2 2001/09/27 16:31:25 uch Exp $	*/

/* Windows CE architecture */

#define __BEGIN_MACRO	do {
#define __END_MACRO	} while (/*CONSTCOND*/0)

#define NAMESPACE_BEGIN(x)	namespace x {
#define NAMESPACE_END		}
#define USING_NAMESPACE(x)	using namespace x;

#include <sys/cdefs.h>
