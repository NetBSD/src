# $NetBSD: Makefile,v 1.5.14.1 2019/06/10 21:51:51 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR} ${COMMON_PATH}

PROG=	wpa_cli
SRCS=	wpa_cli.c wpa_ctrl.c os_unix.c eloop.c edit.c wpa_debug.c common.c
SRCS+=	cli.c

CPPFLAGS+=-DCONFIG_READLINE
LDADD+=-ledit -lterminfo
DPADD+=${LIBEDIT} ${LIBTERMINFO}

MAN=	wpa_cli.8

.include <bsd.prog.mk>
