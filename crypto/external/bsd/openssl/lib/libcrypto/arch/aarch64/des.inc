#	$NetBSD: des.inc,v 1.1.14.1 2018/03/15 09:11:53 pgoyette Exp $

.PATH.S: ${.PARSEDIR}
CPPFLAGS+=	-DDES_INT -DDES_UNROLL
.include "${.CURDIR}/des.inc"
