#	$NetBSD: Makefile,v 1.2 2020/03/01 18:08:13 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/hfs

TESTS_C=	t_pathconvert

FILES=		colon.hfs.bz2.uue
FILESDIR=	${TESTSDIR}

LDADD+=	-lrumpfs_hfs						# fs drivers
LDADD+=	-lrumpdev_disk -lrumpdev				# disk drivers
LDADD+=	${LIBRUMPBASE}

.include <bsd.test.mk>
