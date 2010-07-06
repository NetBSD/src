#	$NetBSD: Makefile,v 1.3 2010/07/06 16:43:48 pooka Exp $

NOATFFILE=	# defined
TESTSDIR=       ${TESTSBASE}/fs/puffs/h_dtfs

TESTS_C=	h_dtfs

SRCS.h_dtfs=	dtfs.c dtfs_vfsops.c dtfs_vnops.c dtfs_subr.c
LDADD+=		-lpuffs -lutil

WARNS=2

.include <bsd.test.mk>
