#	$NetBSD: des.inc,v 1.1.1.2 2023/04/18 14:19:04 christos Exp $

.PATH.S: ${.PARSEDIR}
CPPFLAGS+=	-DDES_INT -DDES_UNROLL
.include "${.CURDIR}/des.inc"
