#	$NetBSD: Makefile,v 1.8.2.2 2026/11/04 22:53:09 perseant Exp $

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
.include "../Makefile.options"

${SRCS}: ${GCCARCH}/defs.mk

COPTS.gcc.cc=	-Wno-error=stack-protector

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/cp
