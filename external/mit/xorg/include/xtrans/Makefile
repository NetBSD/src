#	$NetBSD: Makefile,v 1.3 2014/03/17 01:09:07 mrg Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xtrans}

INCS=	Xtrans.c Xtrans.h Xtransdnet.c Xtransint.h Xtranslcl.c \
	Xtransos2.c Xtranssock.c Xtransutil.c transport.c
INCSDIR=${X11INCDIR}/X11/Xtrans

NOOBJ=	# defined

PKGCONFIG=	xtrans

.include <bsd.x11.mk>
.include <bsd.prog.mk>
