#	$NetBSD: Makefile,v 1.9 2016/06/10 03:37:34 mrg Exp $

# Makefile for modular-xorg-server and drivers

.include <bsd.own.mk>

SUBDIR=	${XORG_SERVER_SUBDIR}

# We only make drivers if we make the actual Xorg server,
# as opposed to Xvfb and Xnest.
.if ${MKXORG_SERVER} != "no"
SUBDIR+= drivers
.endif

.include <bsd.subdir.mk>
