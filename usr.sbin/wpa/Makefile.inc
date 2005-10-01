# $NetBSD: Makefile.inc,v 1.3 2005/10/01 21:34:01 christos Exp $

BINDIR?=	/usr/sbin

WPA_SUPPLICANT_DISTDIR?= ${.CURDIR}/../../../dist/wpa_supplicant
.PATH: ${WPA_SUPPLICANT_DISTDIR}
