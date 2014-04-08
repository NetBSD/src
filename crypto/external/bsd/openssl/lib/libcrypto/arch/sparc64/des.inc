#	$NetBSD: des.inc,v 1.1.8.1 2014/04/08 11:31:20 msaitoh Exp $

.PATH.S: ${.PARSEDIR}
DES_SRCS = des_enc-sparc.S fcrypt_b.c
CPPFLAGS+=	-DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
