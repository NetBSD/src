# $NetBSD: Makefile,v 1.8.4.1 2008/09/28 11:17:10 mjf Exp $

PROG=	cgdconfig
MAN=	cgdconfig.8

SRCS=	cgdconfig.c		\
	cgdlex.l		\
	cgdparse.y		\
	pkcs5_pbkdf2.c		\
	params.c		\
	utils.c

CPPFLAGS+= -I${.CURDIR} -I.

YHEADER=1

DPADD=  ${LIBUTIL} ${LIBCRYPTO} ${LIBCRYPT} ${LIBY} ${LIBL}
LDADD=  -lutil -lcrypto -lcrypt -ly -ll

.include <bsd.prog.mk>
