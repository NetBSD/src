# $NetBSD: Makefile,v 1.1 2005/10/01 18:50:12 scw Exp $

WPA_SUPPLICANT_DISTDIR?= ${.CURDIR}/../../../dist/wpa_supplicant
.PATH: ${WPA_SUPPLICANT_DISTDIR}

PROG=	wpa_cli
SRCS=	wpa_cli.c wpa_ctrl.c

MAN=	wpa_cli.8

.include <bsd.prog.mk>
