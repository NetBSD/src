#	$NetBSD: Makefile,v 1.5.10.1 2026/11/04 23:28:12 perseant Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/nullfs
WARNS=		4

TESTS_SH+=	t_ipc

TESTS_C=	t_basic

LDADD+=	-lrumpfs_tmpfs -lrumpfs_null -lrumpvfs_layerfs		# fs drivers
LDADD+=	${LIBRUMPBASE}						# base

.include <bsd.test.mk>
