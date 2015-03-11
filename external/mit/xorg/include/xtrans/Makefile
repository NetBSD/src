#	$NetBSD: Makefile,v 1.3.4.1 2015/03/11 07:21:15 snj Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xtrans}

INCS=	Xtrans.c Xtrans.h Xtransint.h Xtranslcl.c \
	Xtranssock.c Xtransutil.c transport.c
INCSDIR=${X11INCDIR}/X11/Xtrans

NOOBJ=	# defined

PKGCONFIG=	xtrans

.include <bsd.x11.mk>
.include <bsd.prog.mk>
