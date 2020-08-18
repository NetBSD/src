#	$NetBSD: Makefile,v 1.3 2020/08/18 03:02:50 perseant Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/lfs
WARNS=		4

TESTS_C=	t_pr t_rfw

LDADD+=-lrumpfs_lfs -lrumpfs_ffs				# fs drivers
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=${LIBRUMPBASE}

.include <bsd.test.mk>
