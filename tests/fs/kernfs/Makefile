#	$NetBSD: Makefile,v 1.5 2020/03/01 18:08:13 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/kernfs
WARNS=		4

TESTS_C=	t_basic


LDADD+=	-lrumpfs_kernfs						# fs driver
LDADD+=	${LIBRUMPBASE}

.include <bsd.test.mk>
