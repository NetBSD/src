#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:31 mrg Exp $

PROG=		cc1
SRCS=		${G_C_OBJS:Nlibcpp.a:.o=.c} main.c

.include "../Makefile.backend"
.include "../Makefile.libcpp"

.PATH: ${DIST}/gcc
