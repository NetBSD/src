# $NetBSD: Makefile,v 1.9 2008/07/21 11:38:44 gmcgarry Exp $

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

.if ${HAVE_GCC} == 4 || defined(HAVE_PCC)
COPTS.cgdconfig.c+=	-Wno-pointer-sign
.endif
