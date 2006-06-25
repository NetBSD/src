# $NetBSD: Makefile.inc,v 1.5 2006/06/25 03:02:19 mrg Exp $

BINDIR?=	/usr/sbin

# Don't define .PATH here.

WPA_SUPPLICANT_DISTDIR?=	${.CURDIR}/../../../dist/wpa_supplicant
HOSTAPD_DISTDIR?=		${.CURDIR}/../../../dist/hostapd

.if ${MKCRYPTO} == "no"
NO_OPENSSL=	# defined
.endif
