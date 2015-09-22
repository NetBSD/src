#	$NetBSD: Makefile.inc,v 1.4.38.1 2015/09/22 12:06:16 skrll Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpnet

.include "${RUMPTOP}/Makefile.rump"
