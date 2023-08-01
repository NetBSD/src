#	$NetBSD: Makefile,v 1.12 2023/08/01 05:57:54 mrg Exp $

.include <bsd.init.mk>

PROG=		cpp
SRCS=		cppspec.cc ${G_GCC_OBJS:.o=.cc}

.include "../Makefile.driver"
.include "../Makefile.target-defines"

COPTS+=		-Wno-stack-protector

LINKS=		${BINDIR}/cpp ${BINDIR}/gcpp

MAN=		cpp.1
MLINKS+=	cpp.1 gcpp.1 cpp.1 cccp.1

TEXINFO=	cpp.texi
INFOFLAGS=	-I${DIST}/gcc/doc -I${DIST}/gcc/doc/include -I.

LDADD+=		-lintl
DPADD+=		${LIBINTL}

ADD_FRONTEND_LIBRARY=	no
.include "../Makefile.frontend"

${SRCS}: ${GCCARCH}/defs.mk

.include <bsd.info.mk>

#COPTS.gcc.cc=	-Wno-stack-protector

.PATH: ${DIST}/gcc ${DIST}/gcc/c-family ${DIST}/gcc/doc

cpp.info: gcc-vers.texi
