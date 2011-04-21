#	$NetBSD: Makefile,v 1.2.2.3 2011/04/21 01:42:16 rmind Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/zfs/Makefile.zfsmod"

LIB=	rumpfs_zfs

# RUMP stuff
CPPFLAGS+=      -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpvfs\
		-I${RUMPTOP}/librump/rumpdev

SRCS+=	component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
