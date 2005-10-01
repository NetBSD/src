# $NetBSD: Makefile.inc,v 1.2 2005/10/01 20:46:43 christos Exp $

BINDIR?=	/usr/sbin

WPA_SUPPLICANT_DISTDIR?= ${.CURDIR}/../../../gnu/dist/wpa_supplicant
.PATH: ${WPA_SUPPLICANT_DISTDIR}
