#	$NetBSD: Makefile,v 1.1.46.1 2020/04/08 14:09:06 martin Exp $
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
