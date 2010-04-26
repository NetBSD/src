#	$NetBSD: Makefile,v 1.2 2010/04/26 23:47:25 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/kernfs
WARNS=		4

TESTS_C=	t_basic

.include <bsd.own.mk>

.ifndef RUMPKMOD
LDADD+=	-lrumpfs_kernfs						# fs driver
.endif
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>
