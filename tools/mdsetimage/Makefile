#	$NetBSD: Makefile,v 1.3 2001/10/25 02:37:49 thorpej Exp $

MDSETIMAGE_TARGET_PREFIX=${MACHINE_GNU_PLATFORM}-

HOSTPROGNAME=	${MDSETIMAGE_TARGET_PREFIX}mdsetimage
HOST_SRCDIR=	gnu/usr.sbin/mdsetimage

BFDDIR=	 ${TOOLCHAINOBJDIR}/toolchain/bfd
IBERTYDIR= ${TOOLCHAINOBJDIR}/toolchain/libiberty

.include "${.CURDIR}/../Makefile.host"

TOOLCHAINOBJDIR!= cd ${_CURDIR}/../toolchain && ${PRINTOBJDIR}

HOST_CPPFLAGS+= -I${BFDDIR} -I${DIST}/include

${TIMESTAMP}: ${SRCS}
