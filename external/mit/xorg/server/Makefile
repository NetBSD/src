#	$NetBSD: Makefile,v 1.6.12.1 2013/06/23 06:28:46 tls Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

.include "xorg-server/Makefile.common"

SUBDIR=	xorg-server

# We only make drivers if we make the actual Xorg server,
# as opposed to Xvfb and Xnest.
.if ${MKXORG_SERVER} != "no"
SUBDIR+= drivers
.endif

.include <bsd.subdir.mk>
