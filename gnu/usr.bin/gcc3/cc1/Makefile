#	$NetBSD: Makefile,v 1.2 2003/09/23 09:04:40 matt Exp $

PROG=		cc1
SRCS=		${G_C_OBJS:Nlibcpp.a:.o=.c} main.c

.include "../Makefile.backend"
.include "../Makefile.libcpp"

.PATH: ${DIST}/gcc ${G_out_file:H}
