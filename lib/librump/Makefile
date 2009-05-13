#	$NetBSD: Makefile,v 1.1.8.1 2009/05/13 19:18:36 jym Exp $
#

RUMPTOP=	${.CURDIR}/../../sys/rump

LIBDPLIBS+=     rumpuser ${.CURDIR}/../librumpuser
MAN=		rump.3

WARNS=		3	# XXX: kernel isn't ready for -Wsign-compare

.include "${RUMPTOP}/librump/rumpkern/Makefile.rumpkern"
