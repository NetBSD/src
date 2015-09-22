#	$NetBSD: Makefile.inc,v 1.3.38.1 2015/09/22 12:06:12 skrll Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpdev

.include "${RUMPTOP}/Makefile.rump"
