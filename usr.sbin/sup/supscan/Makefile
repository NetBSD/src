#	$NetBSD: Makefile,v 1.2 1999/02/13 02:55:01 lukem Exp $

PROG=	supscan
SRCS=	supscan.c scan.c
MKMAN=	no

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP}
LDADD=	${SUPLIB}

supscan: .NOPATH

.include <bsd.prog.mk>
