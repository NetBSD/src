#	$NetBSD: Makefile,v 1.3 2001/01/14 10:34:57 christos Exp $

PROG=	supscan
SRCS=	supscan.c scan.c
MKMAN=	no

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP}
LDADD=	${SUPLIB}


.include <bsd.prog.mk>

supscan: .NOPATH
