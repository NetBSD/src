# $NetBSD: Makefile.inc,v 1.6 2006/06/25 03:03:41 mrg Exp $

BINDIR?=	/usr/sbin

# Don't define .PATH here.

WPA_SUPPLICANT_DISTDIR?=	${.CURDIR}/../../../dist/wpa_supplicant
HOSTAPD_DISTDIR?=		${.CURDIR}/../../../dist/hostapd

.include <bsd.own.mk>

.if ${MKCRYPTO} == "no"
NO_OPENSSL=	# defined
.endif
