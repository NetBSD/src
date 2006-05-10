#	$NetBSD: Makefile,v 1.1 2006/05/10 22:48:14 mrg Exp $

.include <bsd.own.mk>

PROG=           gcov
SRCS=		${G_GCOV_OBJS:.o=.c}

CPPFLAGS+=	-I${GCCARCH} -I${BACKENDOBJ} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

BINDIR=		/usr/bin

MAN=		gcov.1

.include <bsd.prog.mk>

LDADD=		-L${LIBIBERTYOBJ} -liberty

.PATH: ${DIST}/gcc
