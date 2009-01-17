#	$NetBSD: Makefile.inc,v 1.1.2.3 2009/01/17 13:29:38 mjf Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpnet/opt

.include "${RUMPTOP}/Makefile.rump"
