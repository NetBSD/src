#	$NetBSD: des.inc,v 1.1.2.2 2016/11/04 14:42:36 pgoyette Exp $

.PATH.S: ${.PARSEDIR}
CPPFLAGS+=	-DDES_INT -DDES_UNROLL
.include "${.CURDIR}/des.inc"
