#	$NetBSD: Makefile,v 1.4 2026/07/11 22:37:16 mrg Exp $

.include <bsd.own.mk>

PROG=	ucs2any

CPPFLAGS+=	-DHAVE_REALLOCARRAY

X11EXTRAMANDEFS+= 	-e 's,__mapfilesdir__,${LIBDIR}/X11/fonts/util,'

.PATH:	${X11SRCDIR.font-util} ${X11SRCDIR.font-util}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
