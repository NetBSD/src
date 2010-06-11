#	$NetBSD: Makefile,v 1.1 2010/06/11 23:52:38 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ptyfs
WARNS=		4
DBG=-g
NOGCCERROR=1

TESTS_C=	t_ptyfs
TESTS_C+=	t_nullpts

LDADD+=	-lrumpfs_ptyfs						# fs drivers
LDADD+=	-lrumpkern_tty						# tty support
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread			# base

LDADD.t_nullpts+= -lrumpfs_null -lrumpvfs_layerfs

.include <bsd.test.mk>
