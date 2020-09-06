#	$NetBSD: Makefile,v 1.11 2020/09/06 07:20:32 mrg Exp $
#

.include <bsd.own.mk>

MOUNTNAME=	lfs

LDADD+=		-lrumpfs_ffs
DPADD+=		${LIBRUMPFS_FFS}

ISRUMP=		# don't deny it
RUMP_DISKFS=

CPPFLAGS+=	-DUSE_RUMP -DLFS_CLEANER_AS_LIB

CWARNFLAGS.clang+=	-Wno-error=absolute-value
CWARNFLAGS.gcc+=        ${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include "../../../libexec/lfs_cleanerd/Makefile.inc"

#DBG=-g

.include <bsd.prog.mk>
