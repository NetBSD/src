#	$NetBSD: Makefile.inc,v 1.6 2022/02/28 16:30:10 joerg Exp $

.if !defined(LLVM_TOPLEVEL_MK)
LLVM_TOPLEVEL_MK=

.include <bsd.own.mk>

COMMON_REVISION=	249b40b558955afe5ac2b549edcf2d7f859c8cc9
CLANG_REVISION=		${COMMON_REVISION}
LLVM_REVISION=		${COMMON_REVISION}

LLVM_VERSION=		13.0.0
CLANG_VERSION=		13.0.0

CLANG_SRCDIR:=	${.PARSEDIR}/dist/clang
LLVM_SRCDIR:=	${.PARSEDIR}/dist/llvm
LLVM_TOPLEVEL:=	${.PARSEDIR}
CONFIG_DIR:=	${.PARSEDIR}/autoconf

CPPFLAGS+=	-I. -I${CLANG_SRCDIR}/include -I${LLVM_SRCDIR}/include

# Bogus warning with -O2 in GCC 7 and 8.
CWARNFLAGS.gcc+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} < 9:? -Wno-stringop-overflow :}

MODULES_CXXFLAGS=	-fmodules -fcxx-modules -Werror=incomplete-umbrella \
			-fmodules-validate-system-headers \
			-Xclang -fmodules-local-submodule-visibility \
			-fmodules-cache-path=${LLVM_MODULE_CACHE:Q}

.if defined(HOSTLIB) || defined(HOSTPROG) || defined(_BSD_HOSTINIT_MK_)
LLVM_INCLUDE_CONFIG=	${LLVM_TOOLCONF_OBJDIR}/config/include
CLANG_INCLUDE_CONFIG=	${LLVM_TOOLCONF_OBJDIR}/config/tools/clang/include
LLVM_INCLUDE_OBJDIR!=	cd ${NETBSDSRCDIR}/tools/llvm-include && ${PRINTOBJDIR}
LLVM_TOOLCONF_OBJDIR!=	cd ${NETBSDSRCDIR}/tools/llvm && ${PRINTOBJDIR}
LLVM_MODULE_CACHE=	${LLVM_TOOLCONF_OBJDIR}/module.cache
HOST_CPPFLAGS+=	${CPPFLAGS}
HOST_CXXFLAGS+=	-O2 -fno-rtti -fno-exceptions -fno-strict-aliasing
.sinclude "${LLVM_TOOLCONF_OBJDIR}/support-modules"
.if ${HOST_SUPPORTS_MODULES:Uno} == "yes"
HOST_CXXFLAGS+=	${MODULES_CXXFLAGS}
.endif
HOST_CPPFLAGS+=	-std=c++14
LLVM_TARGETS=	x86,powerpc,sparc,aarch64,arm,mips
.else
LLVM_INCLUDE_CONFIG=	${LLVM_TOPLEVEL}/config
CLANG_INCLUDE_CONFIG=	${LLVM_TOPLEVEL}/config
LLVM_INCLUDE_OBJDIR!=	cd ${LLVM_TOPLEVEL}/include && ${PRINTOBJDIR}
LLVM_MODULE_CACHE=	${LLVM_INCLUDE_OBJDIR}/module.cache
LLVM_TARGETS=	x86,powerpc,sparc,aarch64,arm,mips,amdgpu
CXXFLAGS+=		${${ACTIVE_CC} == "clang":? ${MODULES_CXXFLAGS} :}

CXXFLAGS+=	-std=c++14
.if ${MKDEBUG:Uno} != "no"
CXXFLAGS+=	-g1
.endif
CXXFLAGS+=	-fno-rtti -fno-exceptions ${${ACTIVE_CC} == "gcc" :? -fno-strict-aliasing :}
CXXFLAGS+=	-ffunction-sections -fdata-sections
LDFLAGS+=	-Wl,--gc-sections
DPADD+=		${LIBEXECINFO}
LDADD+=		-lexecinfo
.endif
CPPFLAGS+=	-I${LLVM_INCLUDE_OBJDIR} -I${LLVM_INCLUDE_CONFIG} \
		-I${CLANG_INCLUDE_CONFIG}

LLVM_PLATFORM=${MACHINE_GNU_ARCH}--netbsd

LLVM_CONFIGURE_ARGS=	\
	--enable-targets=${LLVM_TARGETS} \
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
