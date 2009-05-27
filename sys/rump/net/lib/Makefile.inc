#	$NetBSD: Makefile.inc,v 1.3 2009/05/27 23:41:20 pooka Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpnet -I${RUMPTOP}/librump/rumpnet/opt

.include "${RUMPTOP}/Makefile.rump"
