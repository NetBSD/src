#	$NetBSD: Makefile,v 1.1 2020/07/25 22:53:38 riastradh Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sys/crypto/chacha

TESTS_C=	t_chacha

AFLAGS+=	-D_LOCORE

.PATH:	${NETBSDSRCDIR}/sys/crypto/chacha
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys

SRCS.t_chacha+=	t_chacha.c

SRCS.t_chacha+=	chacha_ref.c
SRCS.t_chacha+=	chacha_selftest.c

.if !empty(MACHINE_ARCH:Maarch64*)

.PATH:	${NETBSDSRCDIR}/sys/crypto/chacha/arch/arm
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/crypto/chacha/arch/arm

SRCS.t_chacha+=	chacha_neon.c
SRCS.t_chacha+=	chacha_neon_64.S
SRCS.t_chacha+=	chacha_neon_impl.c

.endif				# aarch64

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"

.PATH:	${NETBSDSRCDIR}/sys/crypto/chacha/arch/x86
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/crypto/chacha/arch/x86

SRCS.t_chacha+=	chacha_sse2.c
SRCS.t_chacha+=	chacha_sse2_impl.c
COPTS.chacha_sse2.c+=	-msse -msse2

.endif				# x86

WARNS=		5

.include <bsd.test.mk>
