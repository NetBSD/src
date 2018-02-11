#	$NetBSD: Makefile,v 1.7 2018/02/11 02:12:29 christos Exp $

PROG=		gplusplus
PROGNAME=	g++
SRCS=		${G_GXX_OBJS:S/cp\///:.o=.c}

NOMAN=		# Linked to gcc.1
.include <bsd.init.mk>

.include "../Makefile.target-defines"

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++

LDADD+=		-lintl
DPADD+=		${LIBINTL}

.include "../Makefile.frontend"

${SRCS}: ${GCCARCH}/defs.mk

COPTS.gcc.c=	-Wno-stack-protector

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/cp
