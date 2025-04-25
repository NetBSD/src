#	$NetBSD: des.inc,v 1.3 2025/04/25 07:57:42 martin Exp $

.PATH.S: ${.PARSEDIR}
DES_SRCS = des_enc-sparc.S fcrypt_b.c dest4-sparcv9.S
CPPFLAGS+=	-DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
