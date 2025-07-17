#	$NetBSD: des.inc,v 1.1 2025/07/17 14:25:34 christos Exp $

CPPFLAGS+=	-DDES_UNROLL

.include "${.CURDIR}/des.inc"
