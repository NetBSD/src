#	$NetBSD: des.inc,v 1.3 2001/09/10 02:56:58 thorpej Exp $

CPPFLAGS+=	-DDES_UNROLL -DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
