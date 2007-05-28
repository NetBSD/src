# $NetBSD: Makefile.inc,v 1.7 2007/05/28 12:06:42 tls Exp $

BINDIR?=	/usr/sbin

USE_FORT?= yes	# network client/server

# Don't define .PATH here.

WPA_SUPPLICANT_DISTDIR?=	${.CURDIR}/../../../dist/wpa_supplicant
HOSTAPD_DISTDIR?=		${.CURDIR}/../../../dist/hostapd

.include <bsd.own.mk>

.if ${MKCRYPTO} == "no"
NO_OPENSSL=	# defined
.endif
