# $NetBSD: Makefile,v 1.21 2022/05/17 18:56:29 christos Exp $

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
SRCS+=	argon2_utils.c
CPPFLAGS+=	-DHAVE_ARGON2
.if defined(CRUNCHEDPROG)
ARGON2_NO_THREADS=1
.endif
.include "${NETBSDSRCDIR}/external/apache2/argon2/lib/libargon2/Makefile.inc"
.endif

.include <bsd.prog.mk>
