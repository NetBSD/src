#	$NetBSD: Makefile,v 1.4 2008/12/30 22:20:56 pooka Exp $
#

.include <bsd.own.mk>
.undef RUMPKMOD

MOUNTNAME=	ext2fs

LDADD+=		-lrumpfs_ufs -lrumpfs_ffs
DPADD+=		${LIBRUMPFS_UFS} ${LIBRUMPFS_FFS}

ISRUMP=		# yea baby

.include <bsd.prog.mk>
