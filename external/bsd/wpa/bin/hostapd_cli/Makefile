# $NetBSD: Makefile,v 1.2.8.1 2017/08/12 05:56:14 snj Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${HOSTAPD_DISTDIR}

PROG=	hostapd_cli
SRCS=	hostapd_cli.c wpa_ctrl.c os_unix.c eloop.c edit.c wpa_debug.c
SRCS+=	common.c cli.c

MAN=	hostapd_cli.8

.include <bsd.prog.mk>
