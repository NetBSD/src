#	$NetBSD: Makefile,v 1.1 1997/10/07 01:31:21 thorpej Exp $

PROG=	supfilesrv
SRCS=	supfilesrv.c scan.c
MAN=	supservers.8
MLINKS=	supservers.8 supfilesrv.8 supservers.8 supscan.8

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP} ${LIBCRYPT}
LDADD=	${SUPLIB} -lcrypt

supfilesrv: .NOPATH

.include <bsd.prog.mk>
