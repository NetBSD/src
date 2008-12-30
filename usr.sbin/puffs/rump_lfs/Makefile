#	$NetBSD: Makefile,v 1.3 2008/12/30 22:20:56 pooka Exp $
#

.include <bsd.own.mk>
.undef RUMPKMOD

MOUNTNAME=	lfs

LDADD+=		-lrumpfs_lfs -lrumpfs_ufs -lrumpfs_ffs
DPADD+=		${LIBRUMPFS_LFS} ${LIBRUMPFS_UFS} ${LIBRUMPFS_FFS}

ISRUMP=		# don't deny it

.include <bsd.prog.mk>
