#	$NetBSD: Makefile,v 1.6 2001/06/18 16:18:34 tv Exp $

PROG=		cc1obj
SRCS=		${G_OBJC_OBJS:.o=.c}

CPPFLAGS+=	-I${DIST}/gcc/objc

.include "../Makefile.backend"

.PATH: ${DIST}/gcc/objc ${DIST}/gcc
