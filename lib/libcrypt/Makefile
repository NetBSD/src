#	$NetBSD: Makefile,v 1.13 2002/08/19 14:55:15 lukem Exp $

USE_SHLIBDIR=	yes

LIB=	crypt

SRCS=	crypt.c md5crypt.c bcrypt.c blowfish.c

CPPFLAGS+=	-I${.CURDIR}

MAN=	crypt.3
MLINKS= crypt.3 encrypt.3 crypt.3 setkey.3

.include <bsd.lib.mk>
