#	$NetBSD: Makefile,v 1.12 2014/03/02 03:47:57 mrg Exp $

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${NETBSDSRCDIR}/external/gpl3/gcc/dist

BUILDINCS=	basic_file.h c++allocator.h c++io.h c++locale.h
BUILDINCS+=	messages_members.h time_members.h opt_random.h

INCS=		${G_bits_headers} ${G_c_base_headers_extra} ${G_host_headers}
INCS+=		gthr.h gthr-single.h gthr-posix.h gthr-default.h
INCS+=		${BUILDINCS}

# libsupc++
INCS+=		nested_exception.h hash_bytes.h atomic_lockfree_defines.h
INCS+=		cxxabi_forced.h	exception_defines.h exception_ptr.h

.if ${MKCOMPAT} != no
SUBDIR+=	arch
.else
INCS+=		c++config.h
.endif

INCSDIR=	/usr/include/g++/bits

.include "${.CURDIR}/../Makefile.includes"

CLEANFILES+=	{BUILDINCS}

CXXDIST=			${DIST}/libstdc++-v3
COPYHEADERS= \
	opt_random.h		${G_CPU_OPT_BITS_RANDOM} \
	basic_file.h		${G_BASIC_FILE_H} \
	c++allocator.h		${G_ALLOCATOR_H} \
	c++locale.h		${G_CLOCALE_H} \
	c++io.h			${G_CSTDIO_H} \
	messages_members.h	${G_CMESSAGES_H} \
	time_members.h		${G_CTIME_H}

.for _h _s in ${COPYHEADERS}
${_h}: ${CXXDIST}/${_s}
	cp -p ${.ALLSRC} ${.TARGET}
.endfor

.PATH: ${.CURDIR}/../../arch/${GCC_MACHINE_ARCH} \
	${DIST}/libstdc++-v3/include \
	${DIST}/libstdc++-v3/config \
	${DIST}/libstdc++-v3/ \
	${DIST}/libstdc++-v3/libsupc++

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
