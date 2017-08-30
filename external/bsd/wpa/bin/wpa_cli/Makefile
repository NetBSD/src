# $NetBSD: Makefile,v 1.3.8.1 2017/08/30 05:48:08 snj Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR}

PROG=	wpa_cli
SRCS=	wpa_cli.c wpa_ctrl.c os_unix.c eloop.c edit.c wpa_debug.c common.c
SRCS+=	cli.c

CPPFLAGS+=-DCONFIG_READLINE
LDADD+=-ledit -lterminfo
DPADD+=${LIBEDIT} ${LIBTERMINFO}

MAN=	wpa_cli.8

.include <bsd.prog.mk>
