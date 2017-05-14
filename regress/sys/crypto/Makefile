#	$NetBSD: Makefile,v 1.4 2017/05/14 03:36:36 kamil Exp $
#	$KAME: Makefile,v 1.4 2000/11/01 14:55:39 itojun Exp $

SUBDIR=	blowfish cast128 rijndael
.if ${MACHINE_ARCH} == "i386"
SUBDIR+=	des_asm blowfish_asm
.endif

TARGETS+=	benchmark
.PHONY:		benchmark

.include <bsd.subdir.mk>
