#	$NetBSD: Makefile,v 1.3.12.1 2014/08/20 00:04:47 tls Exp $
#

TESTSDIR=	${TESTSBASE}/fs/kernfs
WARNS=		4

TESTS_C=	t_basic

.include <bsd.own.mk>

LDADD+=	-lrumpfs_kernfs						# fs driver
LDADD+=	-lrumpvfs -lrump -lrumpuser -lrump -lpthread		# base

.include <bsd.test.mk>
