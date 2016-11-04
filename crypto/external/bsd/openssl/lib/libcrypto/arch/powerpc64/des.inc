#	$NetBSD: des.inc,v 1.1.30.1 2016/11/04 14:42:36 pgoyette Exp $

CPPFLAGS+=	-DDES_UNROLL

.include "${.CURDIR}/des.inc"
