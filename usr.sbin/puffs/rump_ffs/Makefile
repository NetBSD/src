#	$NetBSD: Makefile,v 1.2 2008/08/05 20:57:46 pooka Exp $
#

MOUNTNAME=	ffs

LDADD+=		-lrumpfs_ffs -lrumpfs_ufs
DPADD+=		${LIBRUMPFS_FFS} ${LIBRUMPFS_UFS}

ISRUMP=		# affirmative

.include <bsd.prog.mk>
