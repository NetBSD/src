#	$NetBSD: Makefile.inc,v 1.2.2.2 2010/03/11 15:04:35 yamt Exp $
#

RUMPTOP=	${.CURDIR}/../../..

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpdev -I${RUMPTOP}/librump/rumpkern

.include "${RUMPTOP}/Makefile.rump"
