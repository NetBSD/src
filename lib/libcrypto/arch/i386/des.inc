#	$NetBSD: des.inc,v 1.3.4.2 2002/08/29 16:40:12 he Exp $

CPPFLAGS+=	-DDES_UNROLL -DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
