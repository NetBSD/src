#	$NetBSD: Makefile,v 1.1 2008/07/29 13:17:47 pooka Exp $
#

PROG=		rump_ext2fs

LDADD+=		-lrumpfs_ext2fs -lrumpfs_ufs -lrumpfs_ffs
DPADD+=		${LIBRUMPFS_EXT2FS} ${LIBRUMPFS_UFS} ${LIBRUMPFS_FFS}

ISRUMP=		# yea baby

.include <bsd.prog.mk>
