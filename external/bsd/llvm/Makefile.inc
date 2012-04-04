#	$NetBSD: Makefile.inc,v 1.32 2012/04/04 10:42:00 joerg Exp $

.if !defined(LLVM_TOPLEVEL_MK)
LLVM_TOPLEVEL_MK=

.include <bsd.own.mk>

LLVM_REVISION=	153935
CLANG_REVISION=	153935

LLVM_SRCDIR:=	${.PARSEDIR}/dist/llvm
CLANG_SRCDIR:=	${.PARSEDIR}/dist/clang
LLVM_TOPLEVEL:=	${.PARSEDIR}

CPPFLAGS+=	-I. -I${LLVM_SRCDIR}/include -I${CLANG_SRCDIR}/include \
		-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS \
		-D__STDC_FORMAT_MACROS

.if defined(HOSTLIB) || defined(HOSTPROG)
LLVM_INCLUDE_CONFIG=	${LLVM_TOOLCONF_OBJDIR}/config/include
CLANG_INCLUDE_CONFIG=	${LLVM_TOOLCONF_OBJDIR}/config/tools/clang/include
LLVM_INCLUDE_OBJDIR!=	cd ${NETBSDSRCDIR}/tools/llvm-include && ${PRINTOBJDIR}
LLVM_TOOLCONF_OBJDIR!=	cd ${NETBSDSRCDIR}/tools/llvm && ${PRINTOBJDIR}
HOST_CPPFLAGS+=	${CPPFLAGS}
HOST_CXXFLAGS+=	-O2 -g
.else
LLVM_INCLUDE_CONFIG=	${LLVM_TOPLEVEL}/config
CLANG_INCLUDE_CONFIG=	${LLVM_TOPLEVEL}/config
LLVM_INCLUDE_OBJDIR!=	cd ${LLVM_TOPLEVEL}/include && ${PRINTOBJDIR}
CPPFLAGS+= -DLLVM_DEFAULT_TARGET_TRIPLE="\"${MACHINE_GNU_PLATFORM}\""
.endif
CPPFLAGS+=	-I${LLVM_INCLUDE_OBJDIR} -I${LLVM_INCLUDE_CONFIG} \
		-I${CLANG_INCLUDE_CONFIG}

# ./configure --enable-targets=x86,powerpc,sparc,arm,mips \
#    --with-c-include-dirs=/usr/include/clang-3.0:/usr/include \
#    --disable-timestamps --prefix=/usr --sysconfdir=/etc/llvm \
#    --disable-threads

.endif
