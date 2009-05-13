#	$NetBSD: Makefile,v 1.2.16.1 2009/05/13 19:20:34 jym Exp $

PROG=	mount_sysctlfs
SRCS=	sysctlfs.c

DPAPP+=	${LIBPUFFS} ${LIBUTIL}
LDADD+=	-lpuffs -lutil

MAN=	mount_sysctlfs.8

.include <bsd.prog.mk>
