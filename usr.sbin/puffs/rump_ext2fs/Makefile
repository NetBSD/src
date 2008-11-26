#	$NetBSD: Makefile,v 1.3 2008/11/26 19:41:11 pooka Exp $
#

MOUNTNAME=	ext2fs

LDADD+=		-lrumpfs_ufs -lrumpfs_ffs
DPADD+=		${LIBRUMPFS_UFS} ${LIBRUMPFS_FFS}

ISRUMP=		# yea baby

.include <bsd.prog.mk>
