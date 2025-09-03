#	$NetBSD: des.inc,v 1.1 2025/07/17 14:25:29 christos Exp $

.PATH.S: ${.PARSEDIR}
CPPFLAGS+=	-DDES_INT -DDES_UNROLL
.include "${.CURDIR}/des.inc"
