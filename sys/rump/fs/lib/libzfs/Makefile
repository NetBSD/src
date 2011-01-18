#	$NetBSD: Makefile,v 1.1 2011/01/18 22:21:23 haad Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/zfs/Makefile.zfsmod"

LIB=	rumpfs_zfs

# RUMP stuff
CPPFLAGS+=      -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpvfs\
		-I${RUMPTOP}/librump/rumpdev
SRCS+= component.c


.include <bsd.lib.mk>
.include <bsd.klinks.mk>
