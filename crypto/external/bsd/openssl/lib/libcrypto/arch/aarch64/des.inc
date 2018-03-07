#	$NetBSD: des.inc,v 1.2 2018/03/07 16:05:44 christos Exp $

.PATH.S: ${.PARSEDIR}
CPPFLAGS+=	-DDES_INT -DDES_UNROLL
.include "${.CURDIR}/des.inc"
