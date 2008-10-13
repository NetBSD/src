#	$NetBSD: Makefile,v 1.2 2008/10/13 16:38:52 cube Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/doc
.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/doc/man

FILES=	README.DRI README.fonts README.rapidaccess

FILESDIR=	${X11USRLIBDIR}/X11/doc

MAN=	Xorg.1 xorg.conf.5

BUILDSYMLINKS+=	${X11SRCDIR.xorg-server}/hw/xfree86/doc/man/Xorg.man.pre \
		Xorg.man \
		${X11SRCDIR.xorg-server}/hw/xfree86/doc/man/xorg.conf.man.pre \
		xorg.conf.man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
