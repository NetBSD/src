#	$NetBSD: Makefile,v 1.12 2020/09/07 03:09:54 mrg Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	lfs

LDADD+=		-lrumpfs_ffs
DPADD+=		${LIBRUMPFS_FFS}

ISRUMP=		# don't deny it
RUMP_DISKFS=

CPPFLAGS+=	-DUSE_RUMP -DLFS_CLEANER_AS_LIB

CWARNFLAGS.clang+=	-Wno-error=absolute-value

.include "../../../libexec/lfs_cleanerd/Makefile.inc"

#DBG=-g

.include <bsd.prog.mk>
