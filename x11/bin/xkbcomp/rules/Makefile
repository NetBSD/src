#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:45 rtr Exp $

.include <bsd.own.mk>

XKBNAME=	rules
FILES=		README sgi xorg sun xfree86 \
		sgi.lst xorg.lst xorg-it.lst sun.lst xfree86.lst xorg.xml

BUILDSYMLINKS=	xorg xfree86 \
		xorg.lst xfree86.lst

.include "../Makefile.xkbcomp"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
