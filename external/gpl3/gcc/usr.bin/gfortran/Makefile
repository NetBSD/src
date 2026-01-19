#	$NetBSD: Makefile,v 1.2 2026/01/19 03:03:55 mrg Exp $

PROG=		gfortran
SRCS=		${G_GFORTRAN_D_OBJS:.o=.cc}

.include "../Makefile.target-defines"

LDADD+=		-lintl
DPADD+=		${LIBINTL}

.include "../Makefile.frontend"
.include "../Makefile.options"

${SRCS}: ${GCCARCH}/defs.mk

.include <bsd.info.mk>

.PATH: ${DIST}/gcc ${DIST}/gcc/doc ${DIST}/gcc/fortran
