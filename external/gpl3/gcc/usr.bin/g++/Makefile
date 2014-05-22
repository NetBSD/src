#	$NetBSD: Makefile,v 1.2.2.1 2014/05/22 16:53:36 yamt Exp $

PROG=		gplusplus
PROGNAME=	g++
SRCS=		${G_GXX_OBJS:.o=.c}

NOMAN=		# Linked to gcc.1
.include <bsd.own.mk>

CPPFLAGS+=	-I${BACKENDOBJ}
# XXX
CPPFLAGS+=	-DCONFIGURE_SPECS="\"\"" \
		-DDEFAULT_TARGET_VERSION=\"${G_version}\" \
		-DDEFAULT_TARGET_MACHINE=\"${MACHINE_GNU_PLATFORM}\"

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++

LDADD+=		-lintl
DPADD+=		${LIBINTL}

.include "../Makefile.frontend"

COPTS.gcc.c=	-Wno-stack-protector

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/cp
