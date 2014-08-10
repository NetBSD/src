#	$NetBSD: Makefile,v 1.4.2.1 2014/08/10 07:10:56 tls Exp $

PROG=		gcc
SRCS=		gccspec.c ${G_GCC_OBJS:.o=.c}

CPPFLAGS+=	-I${BACKENDOBJ}
# XXX
CPPFLAGS+=	-DCONFIGURE_SPECS="\"\"" \
		-DDEFAULT_TARGET_VERSION=\"${G_version}\" \
		-DDEFAULT_TARGET_MACHINE=\"${MACHINE_GNU_PLATFORM}\"

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

.PATH: ${DIST}/gcc ${DIST}/gcc/doc ${DIST}/gcc/c

gcc.info: gcc-vers.texi
