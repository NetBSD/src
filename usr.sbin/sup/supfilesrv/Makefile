#	$NetBSD: Makefile,v 1.4 2001/01/18 03:28:20 mycroft Exp $

PROG=	supfilesrv
SRCS=	supfilesrv.c scan.c
MAN=	supservers.8
MLINKS=	supservers.8 supfilesrv.8 supservers.8 supscan.8

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP} ${LIBCRYPT} ${LIBWRAP}
LDADD=	${SUPLIB} -lcrypt -lwrap

.include <bsd.prog.mk>
