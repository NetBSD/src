#	$NetBSD: des.inc,v 1.2.4.2 2002/08/29 16:39:53 he Exp $

CPPFLAGS+=	-DDES_UNROLL

.include "${.CURDIR}/des.inc"
