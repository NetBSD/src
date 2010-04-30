#	$NetBSD: Makefile.inc,v 1.3.2.1 2010/04/30 14:44:31 uebayasi Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpnet \
		-I${RUMPTOP}/librump/rumpnet/opt

.include "${RUMPTOP}/Makefile.rump"
