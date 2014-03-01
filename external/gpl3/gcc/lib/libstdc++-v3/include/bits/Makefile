#	$NetBSD: Makefile,v 1.9 2014/03/01 10:28:02 mrg Exp $

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${NETBSDSRCDIR}/external/gpl3/gcc/dist

#BUILDINCS=	basic_file.h c++allocator.h c++io.h c++locale.h
#BUILDINCS+=	messages_members.h time_members.h opt_random.h

INCS=		${G_bits_headers} ${G_c_base_headers_extra} ${G_host_headers}
INCS+=		gthr.h gthr-single.h gthr-posix.h gthr-default.h
INCS+=		${G_BASIC_FILE_H} ${G_ALLOCATOR_H} ${G_CLOCALE_H}
INCS+=		${G_CSTDIO_H} ${G_CMESSAGES_H} ${G_CTIME_H}
INCS+=		${G_CPU_OPT_BITS_RANDOM}

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

INCSNAME_${G_BASIC_FILE_H}	= basic_file.h
INCSNAME_${G_ALLOCATOR_H}	= c++allocator.h
INCSNAME_${G_CLOCALE_H}		= c++locale.h
INCSNAME_${G_CSTDIO_H}		= c++io.h
INCSNAME_${G_CMESSAGES_H}	= messages_members.h 
INCSNAME_${G_CTIME_H}		= time_members.h
INCSNAME_${G_CPU_OPT_BITS_RANDOM}	= opt_random.h	

.PATH: ${.CURDIR}/../../arch/${GCC_MACHINE_ARCH} \
	${DIST}/libstdc++-v3/include \
	${DIST}/libstdc++-v3/config \
	${DIST}/libstdc++-v3/ \
	${DIST}/libstdc++-v3/libsupc++

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
