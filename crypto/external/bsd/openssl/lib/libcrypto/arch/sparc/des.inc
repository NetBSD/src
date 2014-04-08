#	$NetBSD: des.inc,v 1.1.8.1 2014/04/08 11:31:19 msaitoh Exp $

.if ${MACHINE} == "sparc64"
.PATH.S: ${.PARSEDIR}
DES_SRCS = des_enc-sparc.S fcrypt_b.c
CPPFLAGS+=     -DDES_UNROLL -DDES_PTR
.else
CPPFLAGS+=     -DDES_UNROLL
.endif

.include "${.CURDIR}/des.inc"
