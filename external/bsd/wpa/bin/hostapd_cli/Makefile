# $NetBSD: Makefile,v 1.1.12.1 2012/11/20 02:58:04 tls Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${HOSTAPD_DISTDIR}

PROG=	hostapd_cli
SRCS=	hostapd_cli.c wpa_ctrl.c os_unix.c eloop.c edit.c wpa_debug.c

MAN=	hostapd_cli.8

.include <bsd.prog.mk>
