#	$NetBSD: Makefile,v 1.1.1.1.4.1 2014/05/22 14:09:36 yamt Exp $

.include <bsd.own.mk>

PROG=	fstobdf
SRCS=	chars.c fstobdf.c header.c props.c

LDADD+=	-lFS -lXext -lX11
DPADD+=	${LIBFS} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
