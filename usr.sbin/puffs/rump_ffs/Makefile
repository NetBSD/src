#	$NetBSD: Makefile,v 1.1 2008/07/29 13:17:47 pooka Exp $
#

PROG=		rump_ffs

LDADD+=		-lrumpfs_ffs -lrumpfs_ufs
DPADD+=		${LIBRUMPFS_FFS} ${LIBRUMPFS_UFS}

ISRUMP=		# affirmative

.include <bsd.prog.mk>
