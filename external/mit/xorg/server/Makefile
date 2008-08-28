#	$NetBSD: Makefile,v 1.4 2008/08/28 07:21:44 mrg Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

.if ${MKXORG_WITH_XSRC_XSERVER} != "no"
SUBDIR= ../../../../x11/Xserver
.else
SUBDIR=	xorg-server drivers
.endif

.include <bsd.subdir.mk>
