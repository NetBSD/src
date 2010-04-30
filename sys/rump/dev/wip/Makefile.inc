#	$NetBSD: Makefile.inc,v 1.1.2.1 2010/04/30 14:44:26 uebayasi Exp $
#

RUMPTOP=	${.CURDIR}/../../..

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpdev -I${RUMPTOP}/librump/rumpkern

.include "${RUMPTOP}/Makefile.rump"
