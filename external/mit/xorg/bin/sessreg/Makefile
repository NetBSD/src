#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

PROG=	sessreg

.include "../xdm/Makefile.xdm"

CPPFLAGS=	-I${X11SRCDIR.${PROG}}/../include

.include <bsd.x11.mk>
.include <bsd.prog.mk>

.PATH:		${X11SRCDIR.${PROG}}
