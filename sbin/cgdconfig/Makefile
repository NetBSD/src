# $NetBSD: Makefile,v 1.5 2005/03/09 03:11:22 christos Exp $

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

DPADD=  ${LIBUTIL} ${LIBCRYPTO} ${LIBCRYPT}
LDADD=  -lutil -lcrypto -lcrypt -ly -ll

.include <bsd.prog.mk>
