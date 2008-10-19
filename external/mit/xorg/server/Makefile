#	$NetBSD: Makefile,v 1.4.6.2 2008/10/19 22:41:14 haad Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

.if ${MKXORG_WITH_XSRC_XSERVER} != "no"
SUBDIR= ../../../../x11/Xserver
.else
SUBDIR=	xorg-server drivers
.endif

.include <bsd.subdir.mk>
