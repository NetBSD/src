#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:04 lukem Exp $

.include <bsd.own.mk>

PROG=	fstobdf
SRCS=	chars.c fstobdf.c header.c props.c

LDADD+=	-lFS -lXext -lX11
DPADD+=	${LIBFS} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
