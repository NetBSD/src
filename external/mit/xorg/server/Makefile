#	$NetBSD: Makefile,v 1.7 2013/06/10 05:01:44 mrg Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

.include "xorg-server/Makefile.common"

SUBDIR=	xorg-server
.if ${MKXORG_DRIVERS} != "no"
SUBDIR+= drivers
.endif

.include <bsd.subdir.mk>
