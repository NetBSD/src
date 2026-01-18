#	$NetBSD: Makefile,v 1.1 2026/01/18 08:08:23 mrg Exp $

PROG=		gfortran
SRCS=		${G_GFORTRAN_D_OBJS:.o=.cc}

.include "../Makefile.target-defines"

NOMAN=		# defined

LDADD+=		-lintl
DPADD+=		${LIBINTL}

.include "../Makefile.frontend"
.include "../Makefile.options"

${SRCS}: ${GCCARCH}/defs.mk

.include <bsd.info.mk>

.PATH: ${DIST}/gcc ${DIST}/gcc/doc ${DIST}/gcc/fortran
