#	$NetBSD: Makefile,v 1.3 2008/12/30 22:20:56 pooka Exp $
#

.include <bsd.own.mk>
.undef RUMPKMOD

MOUNTNAME=	ffs

LDADD+=		-lrumpfs_ffs -lrumpfs_ufs
DPADD+=		${LIBRUMPFS_FFS} ${LIBRUMPFS_UFS}

ISRUMP=		# affirmative

.include <bsd.prog.mk>
