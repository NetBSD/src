#	$NetBSD: Makefile,v 1.10 2024/09/29 17:14:40 kalvisd Exp $

PROG=		gcc
SRCS=		gccspec.cc ${G_GCC_OBJS:.o=.cc}

.include "../Makefile.target-defines"

#COPTS.builtins.cc = -Wno-stack-protector

LINKS+=		${BINDIR}/gcc ${BINDIR}/cc 

MAN=		gcc.1
MLINKS=		gcc.1 cc.1 gcc.1 g++.1 gcc.1 c++.1

TEXINFO=	gcc.texi
INFOFLAGS=	-I${DIST}/gcc/doc -I${DIST}/gcc/doc/include

LDADD+=		-lintl
DPADD+=		${LIBINTL}

.include "../Makefile.frontend"

${SRCS}: ${GCCARCH}/defs.mk

.include <bsd.info.mk>

COPTS.gcc.cc=	-Wno-error=stack-protector

.PATH: ${DIST}/gcc ${DIST}/gcc/doc ${DIST}/gcc/c

gcc.info: gcc-vers.texi
