#	$NetBSD: Makefile,v 1.3 2026/01/21 12:12:15 christos Exp $

PROG=		gfortran
SRCS=		${G_GFORTRAN_D_OBJS:.o=.cc}

.include "../Makefile.target-defines"

LDADD+=		-lintl
DPADD+=		${LIBINTL}

.include "../Makefile.frontend"
.include "../Makefile.options"

${SRCS}: ${GCCARCH}/defs.mk

.include <bsd.info.mk>

COPTS.gcc.cc += -Wno-error=stack-protector

.PATH: ${DIST}/gcc ${DIST}/gcc/doc ${DIST}/gcc/fortran
