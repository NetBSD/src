#	$NetBSD: Makefile,v 1.2 2001/07/18 14:00:12 tv Exp $

.include <bsd.own.mk>

PROG=           g77
SRCS=		g77spec.c version.c

LINKS+=		${BINDIR}/g77 ${BINDIR}/f77 \
		${BINDIR}/g77 ${BINDIR}/fort77

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc/f
