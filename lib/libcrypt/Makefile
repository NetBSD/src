#	$NetBSD: Makefile,v 1.12 2002/05/24 04:02:48 itojun Exp $

LIB=	crypt
USE_SHLIBDIR=	yes

SRCS=	crypt.c md5crypt.c bcrypt.c blowfish.c

CPPFLAGS+=	-I${.CURDIR}

MAN=	crypt.3
MLINKS= crypt.3 encrypt.3 crypt.3 setkey.3

.include <bsd.lib.mk>
