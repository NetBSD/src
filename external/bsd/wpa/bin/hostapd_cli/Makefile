# $NetBSD: Makefile,v 1.3.12.1 2019/01/18 08:48:37 pgoyette Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${HOSTAPD_DISTDIR} ${COMMON_PATH}

PROG=	hostapd_cli
SRCS=	hostapd_cli.c wpa_ctrl.c os_unix.c eloop.c edit.c wpa_debug.c
SRCS+=	common.c cli.c

MAN=	hostapd_cli.8

.include <bsd.prog.mk>
