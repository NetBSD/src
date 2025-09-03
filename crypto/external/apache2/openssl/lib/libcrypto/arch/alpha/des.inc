#	$NetBSD: des.inc,v 1.1 2025/07/17 14:25:28 christos Exp $

CPPFLAGS+=	-DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
