#	$NetBSD: des.inc,v 1.1.1.1 2018/02/03 22:43:37 christos Exp $

CPPFLAGS+=	-DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
