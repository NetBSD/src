#	$NetBSD: Makefile,v 1.3 2002/02/27 02:21:08 itojun Exp $
#	$KAME: Makefile,v 1.4 2000/11/01 14:55:39 itojun Exp $

SUBDIR=	des blowfish cast128 rijndael
.if ${MACHINE_ARCH} == "i386"
SUBDIR+=	des_asm blowfish_asm
.endif

TARGETS+=	benchmark
.PHONY:		benchmark

.include <bsd.subdir.mk>
