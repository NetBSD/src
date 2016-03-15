#	$NetBSD: Makefile,v 1.6 2016/03/15 19:12:06 mrg Exp $

PROG=		gplusplus
PROGNAME=	g++
SRCS=		${G_GXX_OBJS:S/cp\///:.o=.c}

NOMAN=		# Linked to gcc.1
.include <bsd.own.mk>

.include "../Makefile.target-defines"

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++

LDADD+=		-lintl
DPADD+=		${LIBINTL}

.include "../Makefile.frontend"

${SRCS}: ${GCCARCH}/defs.mk

COPTS.gcc.c=	-Wno-stack-protector

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/cp
