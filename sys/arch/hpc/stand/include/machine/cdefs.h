/*	$NetBSD: cdefs.h,v 1.2.20.3 2004/09/18 14:34:46 skrll Exp $	*/

/* Windows CE architecture */

#define	__BEGIN_MACRO	do {
#define	__END_MACRO	} while (/*CONSTCOND*/0)

#define	NAMESPACE_BEGIN(x)	namespace x {
#define	NAMESPACE_END		}
#define	USING_NAMESPACE(x)	using namespace x;
