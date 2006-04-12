# $NetBSD: Makefile.inc,v 1.4 2006/04/12 15:31:51 rpaulo Exp $

BINDIR?=	/usr/sbin

# Don't define .PATH here.

WPA_SUPPLICANT_DISTDIR?=	${.CURDIR}/../../../dist/wpa_supplicant
HOSTAPD_DISTDIR?=		${.CURDIR}/../../../dist/hostapd
