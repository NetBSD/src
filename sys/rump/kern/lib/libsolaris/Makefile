#	$NetBSD: Makefile,v 1.1.14.1 2012/02/18 07:35:46 mrg Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/solaris/Makefile.solmod"

LIB=	rumpkern_solaris

CPPFLAGS+=	-DASSERT=KASSERT

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
