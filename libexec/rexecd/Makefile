#	$NetBSD: Makefile,v 1.6 1997/10/07 10:11:29 mrg Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/4/93

WARNS?=	1
PROG=	rexecd
MAN=	rexecd.8
LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

.include <bsd.prog.mk>
