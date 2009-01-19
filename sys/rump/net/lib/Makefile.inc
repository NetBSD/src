#	$NetBSD: Makefile.inc,v 1.1.8.1 2009/01/19 13:20:27 skrll Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpnet/opt

.include "${RUMPTOP}/Makefile.rump"
