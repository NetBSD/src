#	$NetBSD: Makefile,v 1.2 2001/10/24 23:57:45 thorpej Exp $

MDSETIMAGE_TARGET_PREFIX=${MACHINE_GNU_PLATFORM}-

HOSTPROGNAME=	${MDSETIMAGE_TARGET_PREFIX}mdsetimage
HOST_SRCDIR=	gnu/usr.sbin/mdsetimage

BFDDIR=	 ${TOOLCHAINOBJDIR}/toolchain/bfd
IBERTYDIR= ${TOOLCHAINOBJDIR}/toolchain/libiberty

.include "${.CURDIR}/../Makefile.host"

TOOLCHAINOBJDIR!= cd ${_CURDIR}/../toolchain && ${PRINTOBJDIR}

${TIMESTAMP}: ${SRCS}
