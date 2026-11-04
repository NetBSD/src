#	$NetBSD: Makefile,v 1.4.14.1 2026/11/04 23:15:29 perseant Exp $

.include <bsd.hostinit.mk>

HOSTPROG=	ucs2any
.PATH:		${X11SRCDIR.font-util}

HOST_CPPFLAGS= 	-DDARWIN_NO_CARBON
TOOL_STRING=	"NetBSD tool ucs2any"
HOST_CPPFLAGS= 	-DPACKAGE_STRING=${TOOL_STRING:Q}

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
