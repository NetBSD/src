#	$NetBSD: Makefile,v 1.6 2020/03/01 18:08:14 christos Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ptyfs
WARNS?=		4

TESTS_C=	t_ptyfs
TESTS_C+=	t_nullpts

LDADD+=	-lrumpfs_ptyfs						# fs drivers
LDADD+=	-lrumpkern_tty						# tty support
LDADD+=	${LIBRUMPBASE}						# base

LDADD.t_nullpts+= -lrumpfs_null -lrumpvfs_layerfs

.include <bsd.test.mk>
