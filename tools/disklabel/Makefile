#	$NetBSD: Makefile,v 1.3 2010/01/05 15:45:26 tsutsui Exp $

HOSTPROGNAME=	nbdisklabel-${MAKEWRAPPERMACHINE}
HOST_SRCDIR=	sbin/disklabel
HOST_SRCS=	getcap.c disklabel.c bswap.c

NOMAN=	# defined

.include <bsd.endian.mk>

.if   ${TARGET_ENDIANNESS} == "1234"
CPPFLAGS+= -DTARGET_BYTE_ORDER=LITTLE_ENDIAN
.elif ${TARGET_ENDIANNESS} == "4321"
CPPFLAGS+= -DTARGET_BYTE_ORDER=BIG_ENDIAN
.endif

.include "${.CURDIR}/../Makefile.disklabel"
.include "${.CURDIR}/../Makefile.host"

.PATH: ${.CURDIR}/../../lib/libc/gen
