#	$NetBSD: bn.inc,v 1.4 2003/07/24 14:16:31 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/bn/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bn

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/bn

SRCS+=	bn_add.c bn_div.c bn_exp.c bn_lib.c bn_ctx.c bn_mul.c bn_mod.c \
	bn_print.c bn_rand.c bn_shift.c bn_word.c bn_blind.c \
	bn_kron.c bn_sqrt.c bn_gcd.c bn_prime.c bn_err.c bn_sqr.c bn_asm.c \
	bn_recp.c bn_mont.c bn_mpi.c bn_exp2.c
