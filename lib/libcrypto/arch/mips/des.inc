#	$NetBSD: des.inc,v 1.2 2001/09/11 04:10:41 simonb Exp $

CPPFLAGS+=	-DDES_UNROLL

.include "${.CURDIR}/des.inc"
