#	$NetBSD: Makefile.inc,v 1.2.4.2 2009/05/04 08:14:32 yamt Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpnet/opt

.include "${RUMPTOP}/Makefile.rump"
