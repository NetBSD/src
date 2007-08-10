#	$NetBSD: Makefile,v 1.2 2007/08/10 07:59:32 pooka Exp $

PROG=	mount_sysctlfs
SRCS=	sysctlfs.c

DPAPP+=	${LIBPUFFS} ${LIBUTIL}
LDADD+=	-lpuffs -lutil
WARNS=	4

MAN=	mount_sysctlfs.8

.include <bsd.prog.mk>
