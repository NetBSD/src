#	$NetBSD: Makefile,v 1.2 2003/09/23 09:04:40 matt Exp $

PROG=		cc1obj
SRCS=		${G_OBJC_OBJS:Nlibcpp.a:.o=.c} main.c

CPPFLAGS+=	-I${DIST}/gcc/objc

.include "../Makefile.backend"
.include "../Makefile.libcpp"

.PATH: ${DIST}/gcc/objc ${DIST}/gcc ${G_out_file:H}
