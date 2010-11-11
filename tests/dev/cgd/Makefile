#	$NetBSD: Makefile,v 1.1 2010/11/11 22:38:47 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/cgd

TESTS_SH=	t_cgd

SUBDIR=		h_img2cgd

LDADD+=	-lrumpdev_cgd -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

NOMAN=

.include <bsd.test.mk>
