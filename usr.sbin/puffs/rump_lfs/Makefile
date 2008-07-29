#	$NetBSD: Makefile,v 1.1 2008/07/29 13:17:48 pooka Exp $
#

PROG=		rump_lfs

LDADD+=		-lrumpfs_lfs -lrumpfs_ufs -lrumpfs_ffs
DPADD+=		${LIBRUMPFS_LFS} ${LIBRUMPFS_UFS} ${LIBRUMPFS_FFS}

ISRUMP=		# don't deny it

.include <bsd.prog.mk>
