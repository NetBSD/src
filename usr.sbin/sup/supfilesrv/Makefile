#	$NetBSD: Makefile,v 1.2 1999/08/24 15:52:57 bouyer Exp $

PROG=	supfilesrv
SRCS=	supfilesrv.c scan.c
MAN=	supservers.8
MLINKS=	supservers.8 supfilesrv.8 supservers.8 supscan.8

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP} ${LIBCRYPT}
LDADD=	${SUPLIB} -lcrypt -lwrap

supfilesrv: .NOPATH

.include <bsd.prog.mk>
