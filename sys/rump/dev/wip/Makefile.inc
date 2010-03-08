#	$NetBSD: Makefile.inc,v 1.2 2010/03/08 10:56:12 pooka Exp $
#

RUMPTOP=	${.CURDIR}/../../..

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpdev -I${RUMPTOP}/librump/rumpkern

.include "${RUMPTOP}/Makefile.rump"
