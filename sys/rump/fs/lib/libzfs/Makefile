#	$NetBSD: Makefile,v 1.4.6.2 2016/03/19 11:30:36 skrll Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/zfs/Makefile.zfsmod"

LIB=	rumpfs_zfs
COMMENT=ZFS

SRCS+=	zfs_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
