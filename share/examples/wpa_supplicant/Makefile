#	$NetBSD: Makefile,v 1.1.8.2 2014/08/20 00:02:30 tls Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		wpa_supplicant.conf
FILESDIR=	/usr/share/examples/wpa_supplicant
.endif

.include <bsd.prog.mk>
