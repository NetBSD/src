#	$NetBSD: des.inc,v 1.1.8.2 2015/04/02 17:47:51 snj Exp $

.if ${MACHINE} == "sparc64"
.PATH.S: ${.PARSEDIR}
DES_SRCS = des_enc-sparc.S fcrypt_b.c
CPPFLAGS+=	-DDES_UNROLL -DDES_PTR
.else
CPPFLAGS+=	-DDES_UNROLL
.endif

.include "${.CURDIR}/des.inc"
