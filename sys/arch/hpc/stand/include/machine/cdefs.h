/*	$NetBSD: cdefs.h,v 1.2.20.2 2004/08/12 11:41:11 skrll Exp $	*/

/* Windows CE architecture */

#define	__BEGIN_MACRO	do {
#define	__END_MACRO	} while (/*CONSTCOND*/0)

#define	NAMESPACE_BEGIN(x)	namespace x {
#define	NAMESPACE_END		}
#define	USING_NAMESPACE(x)	using namespace x;
