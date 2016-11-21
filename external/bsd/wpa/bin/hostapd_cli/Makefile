# $NetBSD: Makefile,v 1.3 2016/11/21 20:15:16 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${HOSTAPD_DISTDIR}

PROG=	hostapd_cli
SRCS=	hostapd_cli.c wpa_ctrl.c os_unix.c eloop.c edit.c wpa_debug.c
SRCS+=	common.c cli.c

MAN=	hostapd_cli.8

.include <bsd.prog.mk>
