#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 04:30:11 wrstuden Exp $
#

MOUNTNAME=	ffs

LDADD+=		-lrumpfs_ffs -lrumpfs_ufs
DPADD+=		${LIBRUMPFS_FFS} ${LIBRUMPFS_UFS}

ISRUMP=		# affirmative

.include <bsd.prog.mk>
