# $NetBSD: Makefile,v 1.2 2003/03/24 02:02:49 elric Exp $

PROG=	cgdconfig
MAN=	cgdconfig.8

SRCS=	cgdconfig.c		\
	cgdlex.l		\
	cgdparse.y		\
	pkcs5_pbkdf2.c		\
	params.c		\
	utils.c

CFLAGS+= -I.

YHEADER=1

DPADD=  ${LIBUTIL} ${LIBCRYPTO}
LDADD=  -lutil -lcrypto -ly -ll

.include <bsd.prog.mk>
