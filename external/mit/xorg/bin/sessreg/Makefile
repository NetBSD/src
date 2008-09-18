#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:28 wrstuden Exp $

.include <bsd.own.mk>

PROG=	sessreg

.include "../xdm/Makefile.xdm"

CPPFLAGS=	-I${X11SRCDIR.${PROG}}/../include

.include <bsd.x11.mk>
.include <bsd.prog.mk>

.PATH:		${X11SRCDIR.${PROG}}
