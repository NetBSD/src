/*	$NetBSD: cdefs.h,v 1.4 2004/08/06 18:33:09 uch Exp $	*/

/* Windows CE architecture */

#define	__BEGIN_MACRO	do {
#define	__END_MACRO	} while (/*CONSTCOND*/0)

#define	NAMESPACE_BEGIN(x)	namespace x {
#define	NAMESPACE_END		}
#define	USING_NAMESPACE(x)	using namespace x;
