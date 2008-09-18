#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 04:30:12 wrstuden Exp $
#

MOUNTNAME=	lfs

LDADD+=		-lrumpfs_lfs -lrumpfs_ufs -lrumpfs_ffs
DPADD+=		${LIBRUMPFS_LFS} ${LIBRUMPFS_UFS} ${LIBRUMPFS_FFS}

ISRUMP=		# don't deny it

.include <bsd.prog.mk>
