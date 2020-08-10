#	$NetBSD: Makefile,v 1.8 2020/08/10 09:23:37 mrg Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/doc
.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/man

FILES=	README.modes

FILESDIR=	${X11USRLIBDIR}/X11/doc

MAN=	Xorg.1 xorg.conf.5
# xorg.conf.d.5?

X11EXTRAMANTRANSFORMS_BOTH+= \
	xconfigdir		xorg.conf.d \
	xkbdir			${X11LIBDIR}/xkb \
	modulepath		${X11ROOTDIR}/modules

.include <bsd.x11.mk>
.include <bsd.prog.mk>
