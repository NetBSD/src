#	$NetBSD: Makefile,v 1.1.4.1 2009/05/13 19:18:36 jym Exp $
#

RUMPTOP=	${.CURDIR}/../../sys/rump

LIBDPLIBS+=     rump	${.CURDIR}/../librump

WARNS=		3	# XXX: kernel isn't ready for -Wsign-compare

.include "${RUMPTOP}/librump/rumpnet/Makefile.rumpnet"
