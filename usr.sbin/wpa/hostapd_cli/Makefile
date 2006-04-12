# $NetBSD: Makefile,v 1.1 2006/04/12 15:35:59 rpaulo Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${HOSTAPD_DISTDIR}

PROG=	hostapd_cli
SRCS=	hostapd_cli.c wpa_ctrl.c

MAN=	hostapd_cli.8

.include <bsd.prog.mk>
