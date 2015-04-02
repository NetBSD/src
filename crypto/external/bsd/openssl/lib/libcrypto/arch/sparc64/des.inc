#	$NetBSD: des.inc,v 1.1.14.2 2015/04/02 17:58:49 snj Exp $

.PATH.S: ${.PARSEDIR}
DES_SRCS = des_enc-sparc.S fcrypt_b.c
CPPFLAGS+=	-DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
