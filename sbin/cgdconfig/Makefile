# $NetBSD: Makefile,v 1.14 2010/12/14 17:46:21 pooka Exp $

RUMPPRG=cgdconfig
MAN=	cgdconfig.8

SRCS+=	cgdconfig.c		\
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
