#	$NetBSD: Makefile,v 1.1 2014/01/23 07:26:51 spz Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		wpa_supplicant.conf
FILESDIR=	/usr/share/examples/wpa_supplicant
.endif

.include <bsd.prog.mk>
