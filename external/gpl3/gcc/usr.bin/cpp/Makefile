#	$NetBSD: Makefile,v 1.7 2014/03/22 17:57:48 tron Exp $

PROG=		cpp
SRCS=		cppspec.c ${G_GCC_OBJS:.o=.c}

CPPFLAGS+=	-I${BACKENDOBJ}
# XXX
CPPFLAGS+=	-DCONFIGURE_SPECS="\"\"" \
		-DDEFAULT_TARGET_VERSION=\"${G_version}\" \
		-DDEFAULT_TARGET_MACHINE=\"${MACHINE_GNU_PLATFORM}\"

COPTS+=		-Wno-stack-protector

LINKS=		${BINDIR}/cpp ${BINDIR}/gcpp

MAN=		cpp.1
MLINKS+=	cpp.1 gcpp.1 cpp.1 cccp.1

TEXINFO=	cpp.texi
INFOFLAGS=	-I${DIST}/gcc/doc -I${DIST}/gcc/doc/include -I.

LDADD+=		-lintl
DPADD+=		${LIBINTL}

.include "../Makefile.frontend"

.include <bsd.info.mk>

COPTS.gcc.c=	-Wno-stack-protector

.PATH: ${DIST}/gcc ${DIST}/gcc/c-family ${DIST}/gcc/doc

cpp.info: gcc-vers.texi
