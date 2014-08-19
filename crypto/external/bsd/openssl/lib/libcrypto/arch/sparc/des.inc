#	$NetBSD: des.inc,v 1.1.12.1 2014/08/19 23:45:30 tls Exp $

.if ${MACHINE} == "sparc64"
.PATH.S: ${.PARSEDIR}
DES_SRCS = des_enc-sparc.S fcrypt_b.c
CPPFLAGS+=	-DDES_UNROLL -DDES_PTR
.else
CPPFLAGS+=	-DDES_UNROLL
.endif

.include "${.CURDIR}/des.inc"
