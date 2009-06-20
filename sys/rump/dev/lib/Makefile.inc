#	$NetBSD: Makefile.inc,v 1.1.2.2 2009/06/20 07:20:35 yamt Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpdev/opt

.include "${RUMPTOP}/Makefile.rump"
