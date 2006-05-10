#	$NetBSD: Makefile,v 1.1 2006/05/10 22:53:50 mrg Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../arch/${MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc4
CONF=		${DIST}/libstdc++-v3/config

INCS=		${G_bits_headers} ${G_c_base_headers_extra} ${G_thread_host_headers} ${G_host_headers_extra} ${G_host_headers}
INCSDIR=	/usr/include/g++/bits

CLEANFILES+=	basic_file.h c++io.h c++locale.h

basic_file.h: ${CONF}/io/basic_file_stdio.h
	cp $> ${.TARGET}
c++io.h: ${CONF}/io/c_io_stdio.h
	cp $> ${.TARGET}
c++locale.h: ${CONF}/locale/generic/c_locale.h
	cp $> ${.TARGET}

# List of places to find cpu files; maybe do this differently from
# mknative?
BITS_CPUDIR.sparc64=sparc

_DIR=${BITS_CPUDIR.${MACHINE_ARCH}}
.if ${_DIR} != ""
BITS_CPUDIR=${BITS_CPUDIR.${MACHINE_ARCH}}
.else
BITS_CPUDIR=${MACHINE_GNU_ARCH}
.endif

.PATH:	${DIST}/libstdc++-v3/include/bits \
	${DIST}/libstdc++-v3/include/c_std \
	${CONF}/os/bsd/netbsd \
	${CONF}/io \
	${CONF}/cpu/${BITS_CPUDIR} \
	${CONF}/cpu/generic \
	${CONF}/locale/generic \
	${.CURDIR}/../../arch/${MACHINE_ARCH}

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
