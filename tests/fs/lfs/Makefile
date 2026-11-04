#	$NetBSD: Makefile,v 1.3.8.1 2026/11/04 23:28:12 perseant Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/lfs
WARNS=		4


TESTS_C=	t_basic t_fcntl t_orphan t_pr t_resize t_rfw
SRCS.t_basic+=	util.c t_basic.c
SRCS.t_fcntl+=	util.c t_fcntl.c
SRCS.t_orphan+=	util.c t_orphan.c
SRCS.t_resize+=	util.c t_resize.c
SRCS.t_rfw+=	util.c t_rfw.c

LDADD+=-lrumpfs_lfs -lrumpfs_ffs				# fs drivers
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=${LIBRUMPBASE}

.include <bsd.test.mk>
