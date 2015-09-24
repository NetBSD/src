# $NetBSD: Makefile,v 1.1 2015/09/24 14:12:48 christos Exp $

.include <bsd.own.mk>

MKMAN=	no
LIB=	proc

SRCS=	proc_bkpt.c		\
	proc_create.c		\
	proc_regs.c		\
	proc_sym.c		\
	proc_rtld.c		\
	proc_util.c

INCS=	libproc.h
INCSDIR=/usr/include

LIBPROCDIR=${.CURDIR}/../dist

.PATH: ${LIBPROCDIR}

CPPFLAGS+=	-I${LIBPROCDIR} -I${.CURDIR}/../../librtld_db

.if ${MKCTF} != "no"
OSNET=${NETBSDSRCDIR}/external/cddl/osnet/

LIBDPLIBS +=	ctf ${OSNET}/lib/libctf

CPPFLAGS+=	-I${OSNET}/sys \
		-I${OSNET}/dist/uts/common \
		-I${OSNET}/dist/lib/libctf/common
.else
CPPFLAGS+=	-DNO_CTF
.endif

.if defined(NOCPLUSPLUS)
CPPFLAGS+=	-DNO_CXA_DEMANGLE
.else
. if ${MKLIBCXX} == "yes"
LIBDPLIBS+=     c++     ${NETBSDSRCDIR}/external/bsd/libc++/lib
. else
LIBDPLIBS+=     stdc++  ${NETBSDSRCDIR}/external/gpl3/${EXTERNAL_GCC_SUBDIR}/lib/libstdc++-v3
. endif
.endif

LIBDPLIBS+=	elf ${NETBSDSRCDIR}/external/bsd/elftoolchain/lib/libelf
LIBDPLIBS+=	rtld_db ${NETBSDSRCDIR}/external/bsd/librtld_db/lib
LIBDPLIBS+=	util ${NETBSDSRCDIR}/lib/libutil

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

.include <bsd.lib.mk>
