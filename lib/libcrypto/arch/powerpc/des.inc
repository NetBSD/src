#	$NetBSD: des.inc,v 1.1.4.2 2002/08/29 16:49:32 he Exp $

CPPFLAGS+=	-DDES_UNROLL

.include "${.CURDIR}/des.inc"
