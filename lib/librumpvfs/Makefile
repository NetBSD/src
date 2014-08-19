#	$NetBSD: Makefile,v 1.3.12.1 2014/08/20 00:02:21 tls Exp $
#

RUMPTOP=	${.CURDIR}/../../sys/rump

LIBDPLIBS+=     rump	${.CURDIR}/../librump
MAN=		rump_etfs.3 rumpfs.4

WARNS=		3	# XXX: kernel isn't ready for -Wsign-compare

.include "${RUMPTOP}/librump/rumpvfs/Makefile.rumpvfs"
