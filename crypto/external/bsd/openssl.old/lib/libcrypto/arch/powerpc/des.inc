#	$NetBSD: des.inc,v 1.1.1.1 2018/02/03 22:43:37 christos Exp $

CPPFLAGS+=	-DDES_UNROLL

.include "${.CURDIR}/des.inc"
