# $NetBSD: Makefile,v 1.4 2003/04/17 22:12:50 thorpej Exp $

PROG=	cgdconfig
MAN=	cgdconfig.8

SRCS=	cgdconfig.c		\
	cgdlex.l		\
	cgdparse.y		\
	pkcs5_pbkdf2.c		\
	params.c		\
	utils.c

CFLAGS+= -I${.CURDIR} -I.

YHEADER=1

DPADD=  ${LIBUTIL} ${LIBCRYPTO}
LDADD=  -lutil -lcrypto -ly -ll

.include <bsd.prog.mk>
