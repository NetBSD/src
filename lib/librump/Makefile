#	$NetBSD: Makefile,v 1.4 2010/12/16 15:29:28 pooka Exp $
#

RUMPTOP=	${.CURDIR}/../../sys/rump

LIBDPLIBS+=     rumpuser ${.CURDIR}/../librumpuser
MAN=		rump.3 rump_lwproc.3 rump_sp.7

WARNS=		3	# XXX: kernel isn't ready for -Wsign-compare

.include "${RUMPTOP}/librump/rumpkern/Makefile.rumpkern"
