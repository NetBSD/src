#	$NetBSD: Makefile,v 1.4.2.2 2008/09/18 05:18:40 wrstuden Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

.if ${MKXORG_WITH_XSRC_XSERVER} != "no"
SUBDIR= ../../../../x11/Xserver
.else
SUBDIR=	xorg-server drivers
.endif

.include <bsd.subdir.mk>
