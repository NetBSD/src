# $NetBSD: Makefile,v 1.4 2021/11/22 14:34:35 nia Exp $

LIBISPRIVATE=	pic

.include <bsd.own.mk>

ARGON2DIR=	${NETBSDSRCDIR}/external/apache2/argon2

.PATH: ${ARGON2DIR}/dist/phc-winner-argon2/src \
       ${ARGON2DIR}/dist/phc-winner-argon2/src/blake2 \
       ${ARGON2DIR}/dist/phc-winner-argon2/include

LIB=	argon2
SRCS=	argon2.c core.c blake2b.c thread.c encoding.c ref.c

CFLAGS+=	-pthread
CPPFLAGS+=	-I${ARGON2DIR}/dist/phc-winner-argon2/include

.if ${MACHINE} == "vax"
COPTS.blake2b.c+=	-O0
.endif

.include <bsd.lib.mk>
