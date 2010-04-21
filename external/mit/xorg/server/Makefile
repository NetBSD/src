#	$NetBSD: Makefile,v 1.4.8.1.4.1 2010/04/21 05:24:39 matt Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

.include "xorg-server/Makefile.common"

SUBDIR=	xorg-server
.if ${XSERVER_XFREE86} != "no"
SUBDIR+= drivers
.endif

.include <bsd.subdir.mk>
