#	$NetBSD: Makefile.inc,v 1.2.2.1 2010/04/30 14:44:23 uebayasi Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpdev \
		-I${RUMPTOP}/librump/rumpdev/opt

.include "${RUMPTOP}/Makefile.rump"
