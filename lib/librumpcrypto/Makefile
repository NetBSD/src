#	$NetBSD: Makefile,v 1.1 2009/09/03 12:33:54 pooka Exp $
#

RUMPTOP=	${.CURDIR}/../../sys/rump

LIBDPLIBS+=	rump	${.CURDIR}/../librump

WARNS=		3	# XXX: kernel isn't ready for -Wsign-compare

.include "${RUMPTOP}/librump/rumpcrypto/Makefile.rumpcrypto"
