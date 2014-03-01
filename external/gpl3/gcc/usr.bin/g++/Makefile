#	$NetBSD: Makefile,v 1.3 2014/03/01 10:00:49 mrg Exp $

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

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/cp
