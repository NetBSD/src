#	$NetBSD: Makefile,v 1.8 2001/06/18 16:18:35 tv Exp $

PROG=		cc1plus
SRCS=		${G_CXX_OBJS:.o=.c} c-common.c c-pragma.c

CPPFLAGS+=	-I${DIST}/gcc/cp

.include "../Makefile.backend"

.PATH: ${DIST}/gcc/cp ${DIST}/gcc
