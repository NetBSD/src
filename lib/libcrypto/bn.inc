#	$NetBSD: bn.inc,v 1.2.2.2 2000/06/23 16:16:21 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/bn/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bn

CFLAGS+=	-I${OPENSSLSRC}/crypto/bn

SRCS+=	bn_add.c bn_div.c bn_exp.c bn_lib.c bn_mul.c \
	bn_print.c bn_rand.c bn_shift.c bn_word.c bn_blind.c \
	bn_gcd.c bn_prime.c bn_err.c bn_sqr.c   bn_asm.c  bn_recp.c bn_mont.c \
	bn_mpi.c bn_exp2.c


