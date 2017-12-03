# $NetBSD: Makefile.inc,v 1.2.18.2 2017/12/03 11:35:53 jdolecek Exp $

.if !defined(RUMPKERNEL)
CFLAGS+=	 -mfloat-abi=soft
.endif
