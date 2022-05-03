#	$NetBSD: Makefile,v 1.3 2022/05/03 12:53:52 uwe Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/doc
.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/doc/man

FILES=	README.modes

FILESDIR=	${X11USRLIBDIR}/X11/doc

MAN += Xorg.1
MAN += xorg.conf.5

MLINKS += xorg.conf.5 xorg.conf.d.5

X11EXTRAMANTRANSFORMS+= \
	__xconfigdir__		xorg.conf.d \
	__xkbdir__		${X11LIBDIR}/xkb \
	__modulepath__		${X11ROOTDIR}/modules

.include <bsd.x11.mk>
.include <bsd.prog.mk>
