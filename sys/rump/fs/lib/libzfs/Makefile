#	$NetBSD: Makefile,v 1.9 2023/06/03 08:52:59 lukem Exp $
#

NEED_ISYSTEM=	# Tell rump that we must use -isystem

.include <bsd.init.mk>

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/zfs/Makefile.zfsmod"

LIB=	rumpfs_zfs
COMMENT=ZFS

SRCS+=	zfs_component.c

CWARNFLAGS.gcc+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
