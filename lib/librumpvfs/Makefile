#	$NetBSD: Makefile,v 1.6 2021/11/01 21:37:33 nia Exp $
#

NOFULLRELRO=	yes

RUMPTOP=	${.CURDIR}/../../sys/rump

LIBDPLIBS+=     rump	${.CURDIR}/../librump

WARNS=		3	# XXX: kernel isn't ready for -Wsign-compare

.include "${RUMPTOP}/librump/rumpvfs/Makefile.rumpvfs"
