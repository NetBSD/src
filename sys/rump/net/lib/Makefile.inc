#	$NetBSD: Makefile.inc,v 1.4 2010/03/01 13:12:20 pooka Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpnet \
		-I${RUMPTOP}/librump/rumpnet/opt

.include "${RUMPTOP}/Makefile.rump"
