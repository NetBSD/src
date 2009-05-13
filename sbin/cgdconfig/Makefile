# $NetBSD: Makefile,v 1.10.4.1 2009/05/13 19:19:00 jym Exp $

PROG=	cgdconfig
MAN=	cgdconfig.8

SRCS=	cgdconfig.c		\
	cgdlex.l		\
	cgdparse.y		\
	pkcs5_pbkdf2.c		\
	params.c		\
	utils.c

CPPFLAGS+= -I${.CURDIR} -I. -DYY_NO_INPUT

YHEADER=1

DPADD=  ${LIBUTIL} ${LIBCRYPTO} ${LIBCRYPT} ${LIBY} ${LIBL}
LDADD=  -lutil -lcrypto -lcrypt -ly -ll

.include <bsd.prog.mk>
