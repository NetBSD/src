# $NetBSD: Makefile,v 1.19 2022/05/16 10:44:06 christos Exp $

RUMPPRG=cgdconfig
MAN=	cgdconfig.8

.include <bsd.own.mk>

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

.if ${MKARGON2} != "no"
ARGON2DIR=	${NETBSDSRCDIR}/external/apache2/argon2/dist/phc-winner-argon2
CPPFLAGS+=	-I${ARGON2DIR}/include -I${ARGON2DIR}/src/blake2
CPPFLAGS+=	-DHAVE_ARGON2
.PATH.c: ${ARGON2DIR}/src ${ARGON2DIR}/src/blake2
SRCS+=	argon2_utils.c argon2.c core.c encoding.c ref.c blake2b.c thread.c
DPADD+=  ${LIBPTHREAD}
LDADD+=  -lpthread
.endif

.include <bsd.prog.mk>
