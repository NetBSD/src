#	$NetBSD: Makefile,v 1.1 2001/08/06 19:46:12 tv Exp $

.include <bsd.own.mk>

PROG=           gcov
SRCS=		${G_GCOV_OBJS:.o=.c}
MKMAN=		no

CPPFLAGS+=	-I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

BINDIR=		/usr/bin

.include <bsd.prog.mk>

.PATH: ${DIST}/gcc
