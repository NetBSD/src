#	$NetBSD: Makefile.inc,v 1.2.2.1 2009/07/23 23:32:55 jym Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpnet -I${RUMPTOP}/librump/rumpnet/opt

.include "${RUMPTOP}/Makefile.rump"
