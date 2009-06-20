#	$NetBSD: Makefile.inc,v 1.2.4.3 2009/06/20 07:20:36 yamt Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpnet -I${RUMPTOP}/librump/rumpnet/opt

.include "${RUMPTOP}/Makefile.rump"
