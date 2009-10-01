#	$NetBSD: Makefile.inc,v 1.1 2009/10/01 21:46:30 pooka Exp $
#

RUMPTOP=	${.CURDIR}/../../..

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpdev

.include "${RUMPTOP}/Makefile.rump"
