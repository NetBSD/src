#	$NetBSD: Makefile,v 1.7.4.1 2021/04/17 17:26:22 thorpej Exp $
#

NEED_ISYSTEM=	# Tell rump that we must use -isystem

.include <bsd.init.mk>

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/zfs/Makefile.zfsmod"

LIB=	rumpfs_zfs
COMMENT=ZFS

SRCS+=	zfs_component.c

CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
