#	$NetBSD: Makefile,v 1.3 2009/04/22 15:23:07 lukem Exp $

PROG=	mount_sysctlfs
SRCS=	sysctlfs.c

DPAPP+=	${LIBPUFFS} ${LIBUTIL}
LDADD+=	-lpuffs -lutil

MAN=	mount_sysctlfs.8

.include <bsd.prog.mk>
