#	$NetBSD: Makefile,v 1.6 2018/11/11 22:50:07 mrg Exp $

PROG=		gcc
SRCS=		gccspec.c ${G_GCC_OBJS:.o=.c}

.include "../Makefile.target-defines"

COPTS.builtins.c = -Wno-stack-protector

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

COPTS.gcc.c=	-Wno-stack-protector
.if ${MACHINE_ARCH} == "vax"
COPTS.dse.c=	-O1
.endif

.PATH: ${DIST}/gcc ${DIST}/gcc/doc ${DIST}/gcc/c

gcc.info: gcc-vers.texi
