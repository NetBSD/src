#	$NetBSD: Makefile,v 1.2 2010/12/18 04:31:22 joerg Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ptyfs
WARNS=		4
NOGCCERROR=1

TESTS_C=	t_ptyfs
TESTS_C+=	t_nullpts

LDADD+=	-lrumpfs_ptyfs						# fs drivers
LDADD+=	-lrumpkern_tty						# tty support
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread			# base

LDADD.t_nullpts+= -lrumpfs_null -lrumpvfs_layerfs

.include <bsd.test.mk>
