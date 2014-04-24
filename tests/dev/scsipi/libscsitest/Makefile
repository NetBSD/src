#	$NetBSD: Makefile,v 1.1 2014/04/24 21:46:44 pooka Exp $
#

RUMPTOP= ${NETBSDSRCDIR}/sys/rump

LIB=	rumpdev_scsitest
IOCONF=	SCSITEST.ioconf
LIBISPRIVATE= #defined

SRCS=	scsitest.c
SRCS+=	scsitest_component.c

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
