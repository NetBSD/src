#	$NetBSD: Makefile,v 1.2.2.2 2017/04/10 16:37:49 christos Exp $

.include <bsd.own.mk>

PROG=	dc
SRCS=	main.c dc.c bcode.c inout.c mem.c stack.c

.if ${MKCRYPTO:Uno} == "yes"
WARNS=6
LDADD=	-lcrypto
DPADD=	${LIBCRYPTO}
.else
OPENSSL=${NETBSDSRCDIR}/crypto/external/bsd/openssl/dist
.PATH: ${OPENSSL}/crypto/bn
CPPFLAGS+=-I${OPENSSL} -I${OPENSSL}/crypto
SRCS+= bn_lib.c bn_add.c bn_mul.c bn_div.c bn_asm.c bn_word.c bn_shift.c
SRCS+= bn_ctx.c bn_exp.c bn_mod.c bn_sqr.c bn_mont.c bn_recp.c bn_gcd.c
SRCS+= misc.c
COPTS.bn_lib.c += -Wno-cast-qual
.endif

SUBDIR+=USD.doc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
