#	$NetBSD: Makefile,v 1.3 2001/08/19 14:23:55 mrg Exp $

.include <bsd.own.mk>

PROG=           g77
SRCS=		g77spec.c version.c

LINKS+=		${BINDIR}/g77 ${BINDIR}/f77 \
		${BINDIR}/g77 ${BINDIR}/fort77

MAN=		g77.1
MLINKS=		g77.1 f77.1 \
		g77.1 fort77.1

TEXINFO=	g77.texi
INFOFLAGS=	-I${DIST}/gcc/f

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc/f

.include <bsd.info.mk>
