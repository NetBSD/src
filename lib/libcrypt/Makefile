#	$NetBSD: Makefile,v 1.11.2.1 2004/03/26 22:53:18 jmc Exp $

LIB=	crypt
USE_SHLIBDIR=	yes

SRCS=	crypt.c md5crypt.c bcrypt.c

CPPFLAGS+=	-I${.CURDIR}

MAN=	crypt.3
MLINKS= crypt.3 encrypt.3 crypt.3 setkey.3

.include <bsd.lib.mk>
