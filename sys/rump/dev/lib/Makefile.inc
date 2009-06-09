#	$NetBSD: Makefile.inc,v 1.1 2009/06/09 16:16:15 pooka Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpdev/opt

.include "${RUMPTOP}/Makefile.rump"
