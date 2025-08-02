#	$NetBSD: des.inc,v 1.2.38.1 2025/08/02 05:19:19 perseant Exp $

.PATH.S: ${.PARSEDIR}
DES_SRCS = des_enc-sparc.S fcrypt_b.c dest4-sparcv9.S
CPPFLAGS+=	-DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
