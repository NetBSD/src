#	$NetBSD: des.inc,v 1.1 2001/09/10 02:56:59 thorpej Exp $

CPPFLAGS+=	-DDES_UNROLL

.include "${.CURDIR}/des.inc"
