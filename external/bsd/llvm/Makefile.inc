#	$NetBSD: Makefile.inc,v 1.86 2018/07/17 18:55:24 joerg Exp $

.if !defined(LLVM_TOPLEVEL_MK)
LLVM_TOPLEVEL_MK=

.include <bsd.own.mk>

SVN_ROOT=		http://llvm.org/svn/llvm-project
SVN_BRANCH=		trunk

COMMON_REVISION=	r337282
CLANG_REVISION=		${COMMON_REVISION}
LLD_REVISION=		${COMMON_REVISION}
LLDB_REVISION=		${COMMON_REVISION}
LLVM_REVISION=		${COMMON_REVISION}

MCLINKER_REVISION=	deeb2a77b4165827316f88e0a7ba4ba6b743a080
MCLINKER_ROOT=		https://code.google.com/p/mclinker/

LLVM_VERSION=		7.0.0
CLANG_VERSION=		7.0.0
LLD_VERSION=		7.0.0

CLANG_SRCDIR:=	${.PARSEDIR}/dist/clang
LLD_SRCDIR:=	${.PARSEDIR}/dist/lld
LLDB_SRCDIR:=	${.PARSEDIR}/dist/lldb
LLVM_SRCDIR:=	${.PARSEDIR}/dist/llvm
MCLINKER_SRCDIR:=	${.PARSEDIR}/dist/mclinker
LLVM_TOPLEVEL:=	${.PARSEDIR}
CONFIG_DIR:=	${.PARSEDIR}/dist/llvm-configure

CPPFLAGS+=	-I. -I${CLANG_SRCDIR}/include -I${LLD_SRCDIR}/include \
		-I${LLVM_SRCDIR}/include -I${MCLINKER_SRCDIR}/include \
		-I${LLDB_SRCDIR}/include \
		-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS \
		-D__STDC_FORMAT_MACROS -DCLANG_ENABLE_STATIC_ANALYZER \
		-DENABLE_EXPERIMENTAL_NEW_PASS_MANAGER=false \
		-DCLANG_OPENMP_NVPTX_DEFAULT_ARCH=\"sm_35\" \
		-DCLANG_DEFAULT_OBJCOPY=\"objcopy\"

.if defined(HOSTLIB) || defined(HOSTPROG)
LLVM_INCLUDE_CONFIG=	${LLVM_TOOLCONF_OBJDIR}/config/include
CLANG_INCLUDE_CONFIG=	${LLVM_TOOLCONF_OBJDIR}/config/tools/clang/include
MCLINKER_INCLUDE_CONFIG= ${LLVM_TOPLEVEL}/config/mclinker
LLVM_INCLUDE_OBJDIR!=	cd ${NETBSDSRCDIR}/tools/llvm-include && ${PRINTOBJDIR}
LLVM_TOOLCONF_OBJDIR!=	cd ${NETBSDSRCDIR}/tools/llvm && ${PRINTOBJDIR}
HOST_CPPFLAGS+=	${CPPFLAGS}
HOST_CXXFLAGS+=	-O2 -fno-rtti -fno-exceptions
HOST_CPPFLAGS+=	-std=c++11
.else
LLVM_INCLUDE_CONFIG=	${LLVM_TOPLEVEL}/config
CLANG_INCLUDE_CONFIG=	${LLVM_TOPLEVEL}/config
MCLINKER_INCLUDE_CONFIG= ${LLVM_TOPLEVEL}/config/mclinker
LLVM_INCLUDE_OBJDIR!=	cd ${LLVM_TOPLEVEL}/include && ${PRINTOBJDIR}

.if ${MKLLDB:Uno} == "yes"
CPPFLAGS+=	-DLLDB_DISABLE_PYTHON
.endif
CXXFLAGS+=	-std=c++11
CXXFLAGS+=	-fno-rtti -fno-exceptions
CXXFLAGS+=	-ffunction-sections -fdata-sections
LDFLAGS+=	-Wl,--gc-sections
DPADD+=		${LIBEXECINFO}
LDADD+=		-lexecinfo
.endif
CPPFLAGS+=	-I${LLVM_INCLUDE_OBJDIR} -I${LLVM_INCLUDE_CONFIG} \
		-I${CLANG_INCLUDE_CONFIG} -I${MCLINKER_INCLUDE_CONFIG}

LLVM_PLATFORM=${MACHINE_GNU_ARCH}--netbsd

LLVM_CONFIGURE_ARGS=	\
	--enable-targets=x86,powerpc,sparc,aarch64,arm,mips \
	--with-c-include-dirs=/usr/include/clang-${CLANG_VERSION:R}:/usr/include \
	--disable-timestamps --prefix=/usr --sysconfdir=/etc/llvm \
	--with-clang-default-openmp-runtime=libomp \
	--with-llvm-srcdir=${LLVM_SRCDIR} \
	--with-clang-srcdir=${CLANG_SRCDIR} \
	--host=${LLVM_PLATFORM} --disable-compiler-version-checks \
	--disable-bindings llvm_cv_gnu_make_command=${MAKE}

.for tool in circo dot dotty fdp neato twopi xdot
LLVM_CONFIGURE_ARGS+= ac_cv_path_${tool:tu}="echo ${tool}"
.endfor

.endif
