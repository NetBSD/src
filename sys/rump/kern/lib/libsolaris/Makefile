#	$NetBSD: Makefile,v 1.3 2015/10/19 16:16:37 pooka Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/solaris/Makefile.solmod"

LIB=	rumpkern_solaris
COMMENT=Solaris compatibility layer (for ZFS)

CPPFLAGS+=	-DASSERT=KASSERT

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
