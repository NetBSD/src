#	$NetBSD: Makefile,v 1.4 2001/01/18 03:28:20 mycroft Exp $

PROG=	supscan
SRCS=	supscan.c scan.c
MKMAN=	no

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP}
LDADD=	${SUPLIB}

.include <bsd.prog.mk>
