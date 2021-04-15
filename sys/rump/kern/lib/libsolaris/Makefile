#	$NetBSD: Makefile,v 1.5 2021/04/15 17:28:09 christos Exp $
#

NEED_ISYSTEM=	# Tell rump that we can't use -isystem

.include <bsd.init.mk>

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/solaris/Makefile.solmod"

LIB=	rumpkern_solaris
COMMENT=Solaris compatibility layer (for ZFS)

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
