#	$NetBSD: Makefile,v 1.1.2.2 2009/09/17 04:24:33 snj Exp $

.include <bsd.own.mk>

XKBNAME=	rules
FILES=		README base base.lst base.xml.in \
		evdev evdev.lst evdev.xml.in \
		xfree98 xkb.dtd

FILESNAME_base.xml.in=	base.xml
FILESNAME_evdev.xml.in=	evdev.xml

SYMLINKS=	base ${FILESDIR}/xorg \
		base.lst ${FILESDIR}/xorg.lst \
		base.xml ${FILESDIR}/xorg.xml

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
