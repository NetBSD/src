#	$NetBSD: Makefile,v 1.1 2010/07/06 14:16:44 pooka Exp $

TESTS_C=	h_dtfs

SRCS.h_dtfs=	dtfs.c dtfs_vfsops.c dtfs_vnops.c dtfs_subr.c
LDADD+=		-lpuffs -lutil

WARNS=2
DBG=-g

.include <bsd.test.mk>
