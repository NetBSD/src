#	$NetBSD: Makefile,v 1.2 2011/01/21 13:11:04 pooka Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/zfs/Makefile.zfsmod"

LIB=	rumpfs_zfs

# RUMP stuff
CPPFLAGS+=      -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpvfs\
		-I${RUMPTOP}/librump/rumpdev


.include <bsd.lib.mk>
.include <bsd.klinks.mk>
