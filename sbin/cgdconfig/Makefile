# $NetBSD: Makefile,v 1.10 2008/07/24 05:32:45 dholland Exp $

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
