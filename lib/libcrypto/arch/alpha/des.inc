#	$NetBSD: des.inc,v 1.1.4.2 2002/08/29 16:49:24 he Exp $

CPPFLAGS+=	-DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
