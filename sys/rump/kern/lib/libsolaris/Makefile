#	$NetBSD: Makefile,v 1.4.16.1 2021/04/17 17:26:22 thorpej Exp $
#

NEED_ISYSTEM=	# Tell rump that we must use -isystem

.include <bsd.init.mk>

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/solaris/Makefile.solmod"

LIB=	rumpkern_solaris
COMMENT=Solaris compatibility layer (for ZFS)

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
