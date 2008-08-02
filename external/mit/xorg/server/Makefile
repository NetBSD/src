#	$NetBSD: Makefile,v 1.2 2008/08/02 04:42:19 mrg Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

.if ${MKXORG} != "no"
.if ${MKXORG_WITH_XSRC_XSERVER} != "no"
SUBDIR= ../../../../x11/Xserver
.else
SUBDIR=	xorg-server #drivers
.endif
.endif

.include <bsd.subdir.mk>
