# $NetBSD: Makefile,v 1.1 2002/10/04 18:37:19 elric Exp $

PROG=	cgdconfig
MAN=	cgdconfig.8

SRCS=	cgdconfig.c		\
	pkcs5_pbkdf2.c		\
	params.c		\
	utils.c

DPADD=  ${LIBUTIL} ${LIBCRYPTO}
LDADD=  -lutil -lcrypto

.include <bsd.prog.mk>
