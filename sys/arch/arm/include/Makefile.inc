# $NetBSD: Makefile.inc,v 1.2.2.2 2015/04/06 15:17:52 skrll Exp $

.if !defined(RUMPKERNEL)
CFLAGS+=	 -mfloat-abi=soft
.endif
