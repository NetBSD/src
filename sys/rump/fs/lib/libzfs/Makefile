#	$NetBSD: Makefile,v 1.3.6.1 2014/05/22 11:41:13 yamt Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/zfs/Makefile.zfsmod"

LIB=	rumpfs_zfs

# RUMP stuff
CPPFLAGS+=      -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpvfs\
		-I${RUMPTOP}/librump/rumpdev

SRCS+=	zfs_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
