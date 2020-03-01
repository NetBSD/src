#	$NetBSD: Makefile,v 1.2 2020/03/01 18:08:13 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/lfs
WARNS=		4

TESTS_C=	t_pr

LDADD+=-lrumpfs_lfs -lrumpfs_ffs				# fs drivers
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=${LIBRUMPBASE}

.include <bsd.test.mk>
