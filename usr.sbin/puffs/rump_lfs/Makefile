#	$NetBSD: Makefile,v 1.7 2009/11/05 14:17:07 pooka Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	lfs

LDADD+=		-lrumpfs_ffs
DPADD+=		${LIBRUMPFS_FFS}

ISRUMP=		# don't deny it

CPPFLAGS+=	-DUSE_RUMP -DLFS_CLEANER_AS_LIB

.include "../../../libexec/lfs_cleanerd/Makefile.inc"

DBG=-g

.include <bsd.prog.mk>
