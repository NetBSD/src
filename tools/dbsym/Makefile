#	$NetBSD: Makefile,v 1.1 2001/11/09 05:53:26 thorpej Exp $

DBSYM_TARET_PREFIX=${MACHINE_GNU_PLATFORM}-

HOSTPROGNAME=	${DBSYM_TARET_PREFIX}dbsym
HOST_SRCDIR=	gnu/usr.sbin/dbsym

BFDDIR=	${TOOLCHAINOBJDIR}/toolchain/bfd
IBERTYDIR= ${TOOLCHAINOBJDIR}/toolchain/libiberty

.include "${.CURDIR}/../Makefile.host"

TOOLCHAINOBJDIR!= cd ${_CURDIR}/../toolchain && ${PRINTOBJDIR}

HOST_CPPFLAGS+= -I${BFDDIR} -I${DIST}/include

${TIMESTAMP}: ${SRCS}
