#	$NetBSD: Makefile,v 1.2 2010/05/28 12:38:34 ghen Exp $

.include <bsd.own.mk>

XKBNAME=	rules
FILES=		README base base.lst base.extras.xml.in base.xml.in \
		evdev evdev.lst evdev.extras.xml.in evdev.xml.in \
		xfree98 xkb.dtd

FILESNAME_base.xml.in=		base.xml
FILESNAME_base.extras.xml.in=	base.extras.xml
FILESNAME_evdev.xml.in=		evdev.xml
FILESNAME_evdev.extras.xml.in=	evdev.extras.xml

SYMLINKS=	base ${FILESDIR}/xorg \
		base.lst ${FILESDIR}/xorg.lst \
		base.xml ${FILESDIR}/xorg.xml

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
