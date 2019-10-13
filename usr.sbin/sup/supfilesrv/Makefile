#	$NetBSD: Makefile,v 1.5 2019/10/13 07:28:21 mrg Exp $

PROG=	supfilesrv
SRCS=	supfilesrv.c scan.c
MAN=	supservers.8
MLINKS=	supservers.8 supfilesrv.8 supservers.8 supscan.8

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP} ${LIBCRYPT} ${LIBWRAP}
LDADD=	${SUPLIB} -lcrypt -lwrap

COPTS+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
