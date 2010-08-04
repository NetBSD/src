# $NetBSD: Makefile,v 1.1 2010/08/04 17:12:33 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${HOSTAPD_DISTDIR}

PROG=	hostapd_cli
SRCS=	hostapd_cli.c wpa_ctrl.c os_unix.c

MAN=	hostapd_cli.8

.include <bsd.prog.mk>
