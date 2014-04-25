#	$NetBSD: Makefile,v 1.2 2014/04/25 00:17:48 pooka Exp $
#

RUMPTOP= ${NETBSDSRCDIR}/sys/rump

LIB=	rumpdev_scsitest
IOCONF=	SCSITEST.ioconf
LIBISPRIVATE= #defined

SRCS=	scsitest.c
SRCS+=	scsitest_component.c

CPPFLAGS+= -I${.CURDIR} -I${RUMPTOP}/librump/rumpkern

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
