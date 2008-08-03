#	$NetBSD: Makefile,v 1.3 2008/08/03 06:09:48 rtr Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

.if ${MKXORG} != "no"
.if ${MKXORG_WITH_XSRC_XSERVER} != "no"
SUBDIR= ../../../../x11/Xserver
.else
SUBDIR=	xorg-server drivers
.endif
.endif

.include <bsd.subdir.mk>
