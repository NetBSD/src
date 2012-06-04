#	$NetBSD: Makefile,v 1.5 2012/06/04 18:53:03 joerg Exp $

HOSTPROGNAME=	nbdisklabel-${MAKEWRAPPERMACHINE}
HOST_SRCDIR=	sbin/disklabel
HOST_SRCS=	disklabel.c bswap.c

NOMAN=	# defined

.include <bsd.endian.mk>

.if   ${TARGET_ENDIANNESS} == "1234"
CPPFLAGS+= -DTARGET_BYTE_ORDER=LITTLE_ENDIAN
.elif ${TARGET_ENDIANNESS} == "4321"
CPPFLAGS+= -DTARGET_BYTE_ORDER=BIG_ENDIAN
.endif

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"

.PATH: ${.CURDIR}/../../lib/libc/gen
