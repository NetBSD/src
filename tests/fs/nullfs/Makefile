#	$NetBSD: Makefile,v 1.6 2026/02/28 20:43:28 riastradh Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/nullfs
WARNS=		4

TESTS_SH+=	t_ipc

TESTS_C=	t_basic

LDADD+=	-lrumpfs_tmpfs -lrumpfs_null -lrumpvfs_layerfs		# fs drivers
LDADD+=	${LIBRUMPBASE}						# base

.include <bsd.test.mk>
