#	$NetBSD: Makefile,v 1.6 2009/08/06 01:00:04 pooka Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	lfs
DONOTLINKLIBS=	# FFS and LFS loaded dynamically

ISRUMP=		# don't deny it

CPPFLAGS+=	-DUSE_RUMP -DLFS_CLEANER_AS_LIB

.include "../../../libexec/lfs_cleanerd/Makefile.inc"

DBG=-g

.include <bsd.prog.mk>
