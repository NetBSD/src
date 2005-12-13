#	$NetBSD: des.inc,v 1.2 2005/12/13 09:52:20 martin Exp $

CPPFLAGS+=	-DDES_RISC2 -DDES_PTR

.include "${.CURDIR}/des.inc"
