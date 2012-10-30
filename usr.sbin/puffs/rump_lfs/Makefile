#	$NetBSD: Makefile,v 1.8.6.1 2012/10/30 19:00:50 yamt Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	lfs

LDADD+=		-lrumpfs_ffs
DPADD+=		${LIBRUMPFS_FFS}

ISRUMP=		# don't deny it
RUMP_DISKFS=

CPPFLAGS+=	-DUSE_RUMP -DLFS_CLEANER_AS_LIB

.include "../../../libexec/lfs_cleanerd/Makefile.inc"

#DBG=-g

.include <bsd.prog.mk>
