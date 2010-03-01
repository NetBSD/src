#	$NetBSD: Makefile.inc,v 1.3 2010/03/01 13:12:19 pooka Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpdev \
		-I${RUMPTOP}/librump/rumpdev/opt

.include "${RUMPTOP}/Makefile.rump"
