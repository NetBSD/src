#	$NetBSD: Makefile,v 1.3.4.2 2011/06/06 09:10:06 jruoho Exp $
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
