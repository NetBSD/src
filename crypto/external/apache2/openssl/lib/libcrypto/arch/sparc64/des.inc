#	$NetBSD: des.inc,v 1.1 2025/07/17 14:25:36 christos Exp $

.PATH.S: ${.PARSEDIR}
DES_SRCS = des_enc-sparc.S fcrypt_b.c dest4-sparcv9.S
CPPFLAGS+=	-DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
