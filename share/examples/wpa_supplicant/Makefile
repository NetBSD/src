#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 11:37:45 yamt Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		wpa_supplicant.conf
FILESDIR=	/usr/share/examples/wpa_supplicant
.endif

.include <bsd.prog.mk>
