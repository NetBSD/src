#	$NetBSD: Makefile,v 1.8 2023/08/01 05:57:55 mrg Exp $

PROG=		gplusplus
PROGNAME=	g++
SRCS=		${G_GXX_OBJS:S/cp\///:.o=.cc}

NOMAN=		# Linked to gcc.1
.include <bsd.init.mk>

.include "../Makefile.target-defines"

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++

LDADD+=		-lintl
DPADD+=		${LIBINTL}

.include "../Makefile.frontend"

${SRCS}: ${GCCARCH}/defs.mk

#COPTS.gcc.cc=	-Wno-stack-protector

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/cp
