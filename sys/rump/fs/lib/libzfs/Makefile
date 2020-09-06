#	$NetBSD: Makefile,v 1.7 2020/09/06 07:20:30 mrg Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/zfs/Makefile.zfsmod"

LIB=	rumpfs_zfs
COMMENT=ZFS

SRCS+=	zfs_component.c

CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
