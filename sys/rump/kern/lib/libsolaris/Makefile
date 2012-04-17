#	$NetBSD: Makefile,v 1.1.10.1 2012/04/17 00:08:49 yamt Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/solaris/Makefile.solmod"

LIB=	rumpkern_solaris

CPPFLAGS+=	-DASSERT=KASSERT

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
