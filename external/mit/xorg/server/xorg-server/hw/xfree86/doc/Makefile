#	$NetBSD: Makefile,v 1.1 2008/08/05 04:50:18 mrg Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/doc
.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/doc/man

FILES=	README.DRI README.fonts README.rapidaccess

FILESDIR=	${X11USRLIBDIR}/X11/doc

MAN=	Xorg.1 xorg.conf.5

# XXX
Xorg.1: Xorg.man.pre
	cp ${.ALLSRC} ${.TARGET}

# XXX
xorg.conf.5: xorg.conf.man.pre
	cp ${.ALLSRC} ${.TARGET}
     
.include <bsd.x11.mk>
.include <bsd.prog.mk>
