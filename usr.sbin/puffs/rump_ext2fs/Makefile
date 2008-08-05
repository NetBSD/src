#	$NetBSD: Makefile,v 1.2 2008/08/05 20:57:46 pooka Exp $
#

MOUNTNAME=	ext2fs

LDADD+=		-lrumpfs_ext2fs -lrumpfs_ufs -lrumpfs_ffs
DPADD+=		${LIBRUMPFS_EXT2FS} ${LIBRUMPFS_UFS} ${LIBRUMPFS_FFS}

ISRUMP=		# yea baby

.include <bsd.prog.mk>
