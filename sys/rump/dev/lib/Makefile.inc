#	$NetBSD: Makefile.inc,v 1.1.2.3 2009/09/16 13:38:03 yamt Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpdev -I${RUMPTOP}/librump/rumpdev/opt

.include "${RUMPTOP}/Makefile.rump"
