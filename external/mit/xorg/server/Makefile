#	$NetBSD: Makefile,v 1.5.2.1 2009/05/13 18:53:27 jym Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

.include "xorg-server/Makefile.common"

SUBDIR=	xorg-server
.if ${XSERVER_XFREE86} != "no"
SUBDIR+= drivers
.endif

.include <bsd.subdir.mk>
