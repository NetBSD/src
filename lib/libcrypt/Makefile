#	$NetBSD: Makefile,v 1.14 2003/04/17 00:31:04 thorpej Exp $

USE_SHLIBDIR=	yes

LIB=	crypt

SRCS=	crypt.c md5crypt.c bcrypt.c

CPPFLAGS+=	-I${.CURDIR}

MAN=	crypt.3
MLINKS= crypt.3 encrypt.3 crypt.3 setkey.3

.include <bsd.lib.mk>
