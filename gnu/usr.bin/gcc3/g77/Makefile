#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:36 mrg Exp $

.include <bsd.own.mk>

PROG=           g77
SRCS=		g77spec.c

CPPFLAGS+=	-I${BACKENDOBJ}

LINKS+=		${BINDIR}/g77 ${BINDIR}/f77 \
		${BINDIR}/g77 ${BINDIR}/fort77

MAN=		g77.1
MLINKS=		g77.1 f77.1 \
		g77.1 fort77.1

TEXINFO=	g77.texi
INFOFLAGS=	-I${DIST}/gcc/doc -I${DIST}/gcc/f -I${DIST}/gcc/doc/include

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc/f

.include <bsd.info.mk>
