#	$NetBSD: Makefile,v 1.4.8.2 2009/09/17 04:24:35 snj Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

.include "xorg-server/Makefile.common"

SUBDIR=	xorg-server
.if ${XSERVER_XFREE86} != "no"
SUBDIR+= drivers
.endif

.include <bsd.subdir.mk>
