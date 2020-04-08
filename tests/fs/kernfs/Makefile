#	$NetBSD: Makefile,v 1.4.24.1 2020/04/08 14:09:06 martin Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/kernfs
WARNS=		4

TESTS_C=	t_basic


LDADD+=	-lrumpfs_kernfs						# fs driver
LDADD+=	${LIBRUMPBASE}

.include <bsd.test.mk>
