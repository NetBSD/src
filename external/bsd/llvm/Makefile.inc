#	$NetBSD: Makefile.inc,v 1.50 2013/04/25 13:59:55 joerg Exp $

.if !defined(LLVM_TOPLEVEL_MK)
LLVM_TOPLEVEL_MK=

.include <bsd.own.mk>

SVN_ROOT=		http://llvm.org/svn/llvm-project

COMMON_REVISION=	180004
LLVM_REVISION=		${COMMON_REVISION}
CLANG_REVISION=		${COMMON_REVISION}
COMPILER_RT_REVISION=	${COMMON_REVISION}

MCLINKER_REVISION=	deeb2a77b4165827316f88e0a7ba4ba6b743a080
MCLINKER_ROOT=		https://code.google.com/p/mclinker/

LLVM_VERSION=		3.3
CLANG_VERSION=		3.3

LLVM_SRCDIR:=	${.PARSEDIR}/dist/llvm
CLANG_SRCDIR:=	${.PARSEDIR}/dist/clang
COMPILER_RT_SRCDIR:=	${.PARSEDIR}/dist/compiler-rt
MCLINKER_SRCDIR:=	${.PARSEDIR}/dist/mclinker
LLVM_TOPLEVEL:=	${.PARSEDIR}

CPPFLAGS+=	-I. -I${LLVM_SRCDIR}/include -I${CLANG_SRCDIR}/include \
		-I${MCLINKER_SRCDIR}/include \
		-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS \
		-D__STDC_FORMAT_MACROS

.if defined(HOSTLIB) || defined(HOSTPROG)
LLVM_INCLUDE_CONFIG=	${LLVM_TOOLCONF_OBJDIR}/config/include
CLANG_INCLUDE_CONFIG=	${LLVM_TOOLCONF_OBJDIR}/config/tools/clang/include
MCLINKER_INCLUDE_CONFIG= ${LLVM_TOPLEVEL}/config/mclinker
LLVM_INCLUDE_OBJDIR!=	cd ${NETBSDSRCDIR}/tools/llvm-include && ${PRINTOBJDIR}
LLVM_TOOLCONF_OBJDIR!=	cd ${NETBSDSRCDIR}/tools/llvm && ${PRINTOBJDIR}
HOST_CPPFLAGS+=	${CPPFLAGS}
HOST_CXXFLAGS+=	-O2 -g -fno-rtti -fno-exceptions
.else
LLVM_INCLUDE_CONFIG=	${LLVM_TOPLEVEL}/config
CLANG_INCLUDE_CONFIG=	${LLVM_TOPLEVEL}/config
MCLINKER_INCLUDE_CONFIG= ${LLVM_TOPLEVEL}/config/mclinker
LLVM_INCLUDE_OBJDIR!=	cd ${LLVM_TOPLEVEL}/include && ${PRINTOBJDIR}

CXXFLAGS+=	-fno-rtti -fno-exceptions
.endif
CPPFLAGS+=	-I${LLVM_INCLUDE_OBJDIR} -I${LLVM_INCLUDE_CONFIG} \
		-I${CLANG_INCLUDE_CONFIG} -I${MCLINKER_INCLUDE_CONFIG}

LLVM_CONFIGURE_ARGS=	\
	--enable-targets=x86,powerpc,sparc,aarch64,arm,mips \
	--with-c-include-dirs=/usr/include/clang-${CLANG_VERSION}:/usr/include \
	--disable-timestamps --prefix=/usr --sysconfdir=/etc/llvm \
	--with-clang-srcdir=${CLANG_SRCDIR} \
	--host=${MACHINE_GNU_PLATFORM} \
	llvm_cv_gnu_make_command=${MAKE}

.endif
