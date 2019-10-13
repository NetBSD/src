#	$NetBSD: Makefile,v 1.7 2019/10/13 07:28:21 mrg Exp $

NOMAN=	# defined

PROG=	supscan
SRCS=	supscan.c scan.c

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP}
LDADD=	${SUPLIB}

COPTS+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
