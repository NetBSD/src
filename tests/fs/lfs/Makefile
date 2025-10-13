#	$NetBSD: Makefile,v 1.4 2025/10/13 00:44:35 perseant Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/lfs
WARNS=		4


TESTS_C=	t_basic t_orphan t_pr t_rfw
SRCS.t_basic+=	util.c t_basic.c
SRCS.t_orphan+=	util.c t_orphan.c
SRCS.t_rfw+=	util.c t_rfw.c

LDADD+=-lrumpfs_lfs -lrumpfs_ffs				# fs drivers
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=${LIBRUMPBASE}

.include <bsd.test.mk>
