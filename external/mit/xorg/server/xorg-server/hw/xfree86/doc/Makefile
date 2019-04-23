#	$NetBSD: Makefile,v 1.7.6.1 2019/04/23 10:28:29 martin Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/doc
.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/man

FILES=	README.modes

FILESDIR=	${X11USRLIBDIR}/X11/doc

MAN=	Xorg.1 xorg.conf.5
# xorg.conf.d.5?

X11EXTRAMANTRANSFORMS+= \
	__xconfigdir__		xorg.conf.d \
	__xkbdir__		${X11LIBDIR}/xkb \
	__modulepath__		${X11ROOTDIR}/modules \
	__X11datadir__		${X11LIBDIR}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
