#	$NetBSD: Makefile,v 1.3 2001/01/14 10:36:11 christos Exp $

PROG=	supfilesrv
SRCS=	supfilesrv.c scan.c
MAN=	supservers.8
MLINKS=	supservers.8 supfilesrv.8 supservers.8 supscan.8

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP} ${LIBCRYPT}
LDADD=	${SUPLIB} -lcrypt -lwrap

.include <bsd.prog.mk>

supfilesrv: .NOPATH
