#	$NetBSD: Makefile,v 1.3.6.1 2014/05/22 11:37:00 yamt Exp $
#

RUMPTOP=	${.CURDIR}/../../sys/rump

LIBDPLIBS+=     rump	${.CURDIR}/../librump
MAN=		rump_etfs.3 rumpfs.4

WARNS=		3	# XXX: kernel isn't ready for -Wsign-compare

.include "${RUMPTOP}/librump/rumpvfs/Makefile.rumpvfs"
