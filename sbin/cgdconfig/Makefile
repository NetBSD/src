# $NetBSD: Makefile,v 1.17 2021/11/28 02:01:30 christos Exp $

RUMPPRG=cgdconfig
MAN=	cgdconfig.8

.include <bsd.own.mk>

SRCS+=	cgdconfig.c		\
	cgdlex.l		\
	cgdparse.y		\
	pkcs5_pbkdf2.c		\
	params.c		\
	utils.c

.if ${MKARGON2} != "no"
SRCS+=	argon2_utils.c
.endif

CPPFLAGS+= -I${.CURDIR} -I. -DYY_NO_INPUT

YHEADER=1

DPADD=  ${LIBUTIL} ${LIBCRYPT} ${LIBY} ${LIBL}
LDADD=  -lutil -lcrypt -ly -ll

.if ${MKARGON2} != "no"
ARGON2DIR=	${NETBSDSRCDIR}/external/apache2/argon2
ARGON2OBJDIR!=	cd ${ARGON2DIR}/lib/libargon2 && ${PRINTOBJDIR}
CPPFLAGS+=	-I${NETBSDSRCDIR}/external/apache2/argon2/dist/phc-winner-argon2/include
CPPFLAGS+=	-DHAVE_ARGON2

PROGDPLIBS+=	argon2 ${ARGON2DIR}/lib/libargon2

LDADD+=		-lpthread
DPADD+=		${LIBPTHREAD}
.endif

.include <bsd.prog.mk>
