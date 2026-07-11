#	$NetBSD: Makefile,v 1.5 2026/07/11 22:37:16 mrg Exp $

.include <bsd.hostinit.mk>

HOSTPROG=	ucs2any
.PATH:		${X11SRCDIR.font-util}

HOST_CPPFLAGS= 	-DDARWIN_NO_CARBON
TOOL_STRING=	"NetBSD tool ucs2any"
HOST_CPPFLAGS= 	-DPACKAGE_STRING=${TOOL_STRING:Q}

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
