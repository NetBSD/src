#	$NetBSD: Makefile,v 1.7 1997/10/07 11:31:21 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

WARNS?=	1
PROG=	makekey
MAN=	makekey.8
LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

.include <bsd.prog.mk>
