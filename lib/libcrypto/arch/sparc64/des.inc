#	$NetBSD: des.inc,v 1.1 2001/09/10 02:57:00 thorpej Exp $

CPPFLAGS+=	-DDES_UNROLL -DDES_RISC1 -DDES_PTR

.include "${.CURDIR}/des.inc"
