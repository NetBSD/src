#	$NetBSD: Makefile,v 1.3.20.1 2014/05/18 17:46:17 rmind Exp $
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
