#	$NetBSD: Makefile,v 1.1 1997/10/07 01:31:24 thorpej Exp $

PROG=	supscan
SRCS=	supscan.c scan.c
NOMAN=

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP}
LDADD=	${SUPLIB}

supscan: .NOPATH

.include <bsd.prog.mk>
