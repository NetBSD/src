#	$NetBSD: Makefile,v 1.4 2020/08/16 15:52:14 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sys/crypto/chacha

TESTS_C=	t_chacha

AFLAGS+=	-D_LOCORE

.PATH:	${NETBSDSRCDIR}/sys/crypto/chacha
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys

SRCS.t_chacha+=	t_chacha.c

SRCS.t_chacha+=	chacha_ref.c
SRCS.t_chacha+=	chacha_selftest.c

.if !empty(MACHINE_ARCH:Mearmv7hf*) || !empty(MACHINE_ARCH:Maarch64*)

.PATH:	${NETBSDSRCDIR}/sys/crypto/chacha/arch/arm
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/crypto/chacha/arch/arm

SRCS.t_chacha+=	chacha_neon.c
.if !empty(MACHINE_ARCH:Mearmv7hf*)
SRCS.t_chacha+=	chacha_neon_32.S
.elif !empty(MACHINE_ARCH:Maarch64*)
SRCS.t_chacha+=	chacha_neon_64.S
.endif
SRCS.t_chacha+=	chacha_neon_impl.c

.endif				# earmv7 or aarch64

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"

.PATH:	${NETBSDSRCDIR}/sys/crypto/chacha/arch/x86
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/crypto/chacha/arch/x86

SRCS.t_chacha+=	chacha_sse2.c
SRCS.t_chacha+=	chacha_sse2_impl.c
COPTS.chacha_sse2.c+=	-msse -msse2

.endif				# x86

WARNS=		5

.include <bsd.test.mk>
