#	$NetBSD: Makefile,v 1.4.18.1 2014/08/10 06:57:07 tls Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ptyfs
WARNS?=		4

TESTS_C=	t_ptyfs
TESTS_C+=	t_nullpts

LDADD+=	-lrumpfs_ptyfs						# fs drivers
LDADD+=	-lrumpkern_tty						# tty support
LDADD+=	-lrumpvfs -lrump -lrumpuser -lrump -lpthread		# base

LDADD.t_nullpts+= -lrumpfs_null -lrumpvfs_layerfs

.include <bsd.test.mk>
