#	$NetBSD: Makefile,v 1.3.16.2 2017/12/03 11:39:13 jdolecek Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/zfs/Makefile.zfsmod"

LIB=	rumpfs_zfs
COMMENT=ZFS

SRCS+=	zfs_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
