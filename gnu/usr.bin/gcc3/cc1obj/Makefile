#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:32 mrg Exp $

PROG=		cc1obj
SRCS=		${G_OBJC_OBJS:Nlibcpp.a:.o=.c} main.c

CPPFLAGS+=	-I${DIST}/gcc/objc

.include "../Makefile.backend"
.include "../Makefile.libcpp"

.PATH: ${DIST}/gcc/objc ${DIST}/gcc
