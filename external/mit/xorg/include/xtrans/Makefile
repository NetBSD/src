#	$NetBSD: Makefile,v 1.4.2.1 2017/01/07 08:55:59 pgoyette Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xtrans}

INCS=	Xtrans.c Xtrans.h Xtransint.h Xtranslcl.c \
	Xtranssock.c Xtransutil.c transport.c
INCSDIR=${X11INCDIR}/X11/Xtrans

NOOBJ=	# defined

PKGCONFIG=	xtrans

FILES=		xtrans.m4
FILESDIR=	${X11ROOTDIR}/share/aclocal

.include <bsd.x11.mk>
.include <bsd.prog.mk>
