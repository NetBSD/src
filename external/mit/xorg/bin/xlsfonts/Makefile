#	$NetBSD: Makefile,v 1.2 2015/07/19 23:15:05 mrg Exp $

.include <bsd.own.mk>

PROG=	xlsfonts
SRCS=	xlsfonts.c dsimple.c

LDADD+=	-lXext -lX11
DPADD+=	${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

COPTS.xlsfonts.c+=      -Wno-error

.include <bsd.x11.mk>
.include <bsd.prog.mk>
