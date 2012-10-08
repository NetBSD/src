# $NetBSD: Makefile,v 1.2 2012/10/08 00:03:20 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${HOSTAPD_DISTDIR}

PROG=	hostapd_cli
SRCS=	hostapd_cli.c wpa_ctrl.c os_unix.c eloop.c edit.c wpa_debug.c

MAN=	hostapd_cli.8

.include <bsd.prog.mk>
