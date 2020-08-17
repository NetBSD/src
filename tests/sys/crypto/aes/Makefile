#	$NetBSD: Makefile,v 1.5 2020/08/17 16:26:02 riastradh Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sys/crypto/aes

TESTS_C=	t_aes

AFLAGS+=	-D_LOCORE

.PATH:	${NETBSDSRCDIR}/sys/crypto/aes
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys

SRCS.t_aes+=	t_aes.c

SRCS.t_aes+=	aes_bear.c
SRCS.t_aes+=	aes_ct.c
SRCS.t_aes+=	aes_ct_dec.c
SRCS.t_aes+=	aes_ct_enc.c
SRCS.t_aes+=	aes_selftest.c

.if !empty(MACHINE_ARCH:Mearmv7*) || !empty(MACHINE_ARCH:Maarch64*)

.PATH:	${NETBSDSRCDIR}/sys/crypto/aes/arch/arm
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/crypto/aes/arch/arm

.if !empty(MACHINE_ARCH:Maarch64*)	# XXX no AESE/AESD in 32-bit mode yet
SRCS.t_aes+=	aes_armv8.c
SRCS.t_aes+=	aes_armv8_64.S
.endif

SRCS.t_aes+=	aes_neon.c
SRCS.t_aes+=	aes_neon_impl.c
SRCS.t_aes+=	aes_neon_subr.c
.if !empty(MACHINE_ARCH:Mearmv7*)
SRCS.t_aes+=	aes_neon_32.S
.endif

.if !empty(MACHINE_ARCH:Mearmv7*) && empty(MACHINE_ARCH:Mearmv7hf*)
COPTS.aes_neon.c+=	-mfloat-abi=softfp -mfpu=neon
COPTS.aes_neon_subr.c+=	-mfloat-abi=softfp -mfpu=neon
AOPTS.aes_neon_32.S+=	-D__SOFTFP__
.endif

.endif				# earmv7/aarch64

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"

.PATH:	${NETBSDSRCDIR}/sys/crypto/aes/arch/x86
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/crypto/aes/arch/x86

.if ${MACHINE_ARCH} == "x86_64"		# XXX no AES-NI in 32-bit mode yet
SRCS.t_aes+=	aes_ni.c
SRCS.t_aes+=	aes_ni_64.S
.endif

SRCS.t_aes+=	aes_sse2.c
SRCS.t_aes+=	aes_sse2_dec.c
SRCS.t_aes+=	aes_sse2_enc.c
SRCS.t_aes+=	aes_sse2_impl.c
SRCS.t_aes+=	aes_sse2_subr.c
COPTS.aes_sse2.c+=	-msse -msse2
COPTS.aes_sse2_dec.c+=	-msse -msse2
COPTS.aes_sse2_enc.c+=	-msse -msse2
COPTS.aes_sse2_subr.c+=	-msse -msse2

SRCS.t_aes+=	aes_ssse3.c
SRCS.t_aes+=	aes_ssse3_impl.c
SRCS.t_aes+=	aes_ssse3_subr.c
COPTS.aes_ssse3.c+=		-msse -msse2 -msse3 -mssse3
COPTS.aes_ssse3_subr.c+=	-msse -msse2 -msse3 -mssse3

SRCS.t_aes+=	aes_via.c

.endif				# x86

WARNS=		5

.include <bsd.test.mk>
