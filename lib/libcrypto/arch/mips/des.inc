#	$NetBSD: des.inc,v 1.1 2001/09/10 02:56:59 thorpej Exp $

CPPFLAGS+=	-DDES_UNROLL -DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
