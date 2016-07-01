# $NetBSD: Makefile,v 1.15 2016/07/01 22:50:09 christos Exp $

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

DPADD=  ${LIBUTIL} ${LIBCRYPT} ${LIBY} ${LIBL}
LDADD=  -lutil -lcrypt -ly -ll

.include <bsd.prog.mk>
