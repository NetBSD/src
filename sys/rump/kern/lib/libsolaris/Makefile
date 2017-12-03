#	$NetBSD: Makefile,v 1.2.6.1 2017/12/03 11:39:15 jdolecek Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/solaris/Makefile.solmod"

LIB=	rumpkern_solaris
COMMENT=Solaris compatibility layer (for ZFS)

CPPFLAGS+=	-DASSERT=KASSERT

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
