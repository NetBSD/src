# $NetBSD: Makefile,v 1.3 2003/04/17 10:55:43 fvdl Exp $

PROG=	cgdconfig
MAN=	cgdconfig.8

SRCS=	cgdconfig.c		\
	cgdlex.l		\
	cgdparse.y		\
	pkcs5_pbkdf2.c		\
	params.c		\
	utils.c

CFLAGS+= -I${.CURDIR}

YHEADER=1

DPADD=  ${LIBUTIL} ${LIBCRYPTO}
LDADD=  -lutil -lcrypto -ly -ll

.include <bsd.prog.mk>
