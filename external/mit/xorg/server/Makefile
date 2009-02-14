#	$NetBSD: Makefile,v 1.6 2009/02/14 12:42:48 abs Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

.include "xorg-server/Makefile.common"

SUBDIR=	xorg-server
.if ${XSERVER_XFREE86} != "no"
SUBDIR+= drivers
.endif

.include <bsd.subdir.mk>
