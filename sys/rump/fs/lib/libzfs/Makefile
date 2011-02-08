#	$NetBSD: Makefile,v 1.1.2.1 2011/02/08 16:20:03 bouyer Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/zfs/Makefile.zfsmod"

LIB=	rumpfs_zfs

# RUMP stuff
CPPFLAGS+=      -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpvfs\
		-I${RUMPTOP}/librump/rumpdev


.include <bsd.lib.mk>
.include <bsd.klinks.mk>
