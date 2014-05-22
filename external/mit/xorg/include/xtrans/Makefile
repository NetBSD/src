#	$NetBSD: Makefile,v 1.2.4.1 2014/05/22 14:09:40 yamt Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xtrans}

INCS=	Xtrans.c Xtrans.h Xtransdnet.c Xtransint.h Xtranslcl.c \
	Xtransos2.c Xtranssock.c Xtransutil.c transport.c
INCSDIR=${X11INCDIR}/X11/Xtrans

NOOBJ=	# defined

PKGCONFIG=	xtrans

.include <bsd.x11.mk>
.include <bsd.prog.mk>
