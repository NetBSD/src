#	$NetBSD: Makefile.inc,v 1.1.2.4 2010/03/11 15:04:32 yamt Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpdev \
		-I${RUMPTOP}/librump/rumpdev/opt

.include "${RUMPTOP}/Makefile.rump"
