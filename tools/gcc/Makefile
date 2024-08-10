#	$NetBSD: Makefile,v 1.114 2024/08/10 23:49:12 mrg Exp $

.include <bsd.hostinit.mk>

GCC_LANGUAGES=c c++ objc
MODULE=		${EXTERNAL_GCC_SUBDIR}

GCCDIST=	${.CURDIR}/../../external/gpl3/${EXTERNAL_GCC_SUBDIR}/dist
GNUHOSTDIST=	${GCCDIST}

.if ${HAVE_GCC} == 10
MKNATIVE=		${.CURDIR}/mknative-gcc.old
.endif

# Defaults
CC_FOR_BUILD=		${HOST_CXX:Q}
MKNATIVE?=		${.CURDIR}/mknative-gcc

# Force C++11 so builds succeed on hosts where this isn't the compiler
# default, e.g., NetBSD 8.x with GCC 5.5.
.if ${HAVE_GCC} > 10
HOST_CXXFLAGS+=		-std=c++11
.endif

.if ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "sparc64"
MULTILIB_ARGS= --enable-multilib
.else
MULTILIB_ARGS= --disable-multilib
.endif

.if ${MKSOFTFLOAT} != "no" && ${MACHINE_CPU} != "m68k" \
    && ${MACHINE_CPU} != "or1k" && ${MACHINE_CPU} != "sh3"
SOFTFLOAT_ARGS=	-with-float=soft
.endif

.include "gcc-version.mk"

COMMON_CONFIGURE_ARGS=	--target=${MACHINE_GNU_PLATFORM} \
			--enable-long-long \
			--enable-threads \
			--with-bugurl=http://www.NetBSD.org/support/send-pr.html \
			--with-pkgversion="NetBSD ${NETBSD_GCC_VERSION}" \
			--with-system-zlib \
			--enable-__cxa_atexit \
			--enable-libstdcxx-time=rt \
			--enable-libstdcxx-threads \
			--with-diagnostics-color=auto-if-env
.if defined(GCC_CONFIG_ARCH.${MACHINE_ARCH})
COMMON_CONFIGURE_ARGS+=	--with-arch=${GCC_CONFIG_ARCH.${MACHINE_ARCH}}
.endif
.if defined(GCC_CONFIG_TUNE.${MACHINE_ARCH})
COMMON_CONFIGURE_ARGS+=	--with-tune=${GCC_CONFIG_TUNE.${MACHINE_ARCH}}
.endif

# Too many CPUs have one or both of these bugs, the common code should
# default to safety.
.if ${MACHINE_CPU} == "aarch64"
COMMON_CONFIGURE_ARGS+=	--enable-fix-cortex-a53-835769 \
			--enable-fix-cortex-a53-843419
.endif

.if ${HAVE_GCC} >= 7
COMMON_CONFIGURE_ARGS+=	--with-default-libstdcxx-abi=new
.endif

# We enabled isl support for GCC 12.  Move into normal segment when
# removing GCC 10.
.if ${HAVE_GCC} < 12 || defined(NOGCCISL)
COMMON_CONFIGURE_ARGS+=	--without-isl
.else
COMMON_CONFIGURE_ARGS+=	--with-isl=${TOOLDIR}
.endif

CONFIGURE_ARGS=	${COMMON_CONFIGURE_ARGS}
CONFIGURE_ARGS+= \
		--with-sysroot=${DESTDIR} \
		--with-mpc=${TOOLDIR} \
		--with-mpfr=${TOOLDIR} \
		--with-gmp=${TOOLDIR} \
		${ISL_CONFIGURE_ARGS} \
		--disable-nls \
		${MULTILIB_ARGS} \
		${SOFTFLOAT_ARGS} \
		--program-transform-name="s,^,${MACHINE_GNU_PLATFORM}-," \
		--enable-languages="${GCC_LANGUAGES}"

GCC_CPPFLAGS=	-DNETBSD_TOOLS -DTARGET_SYSTEM_ROOT=0 \
		-DTARGET_SYSTEM_ROOT_RELOCATABLE 
#		${HOST_CPPFLAGS}

.if ${MACHINE} == "vax"
GCC_CPPFLAGS+=	-DNB_FIX_VAX_BACKEND
.endif

MAKE_ARGS=	MACHINE= MAKEINFO=${TOOL_MAKEINFO:Q} \
		LIBGCC= LIBGCC1= LIBGCC1_TEST= LIBGCC2= INSTALL_LIBGCC= \
		EXTRA_PARTS= CPPFLAGS=${GCC_CPPFLAGS:Q} \
		AR=${HOST_AR:Q} RANLIB=${HOST_RANLIB:Q}

CONFIGURE_ENV+= gcc_cv_libc_provides_ssp=yes \
		gcc_cv_as_sparc_gotdata_op=no

ALL_TARGET=	all-gcc
INSTALL_TARGET=	install-gcc

.include "${.CURDIR}/../Makefile.gmakehost"
BUILD_MAKE=${TOOL_GMAKE}

#
# mknative-gcc specific stuff
#

MKNATIVE_CONFIG_TARGET_LIBS=

MKNATIVE_CONFIG_TARGET_LIBS+=	configure-target-libgcc
MKNATIVE_CONFIG_TARGET_LIBS+=	configure-target-libgomp

MKNATIVE_CONFIG_TARGET_LIBS+= \
	configure-target-libstdc++-v3 \
	configure-target-libbacktrace \
	configure-target-libobjc

.if ${HAVE_GCC} >= 9
# XXX needs to build libstdc++ fully, which fails
.if 0
MKNATIVE_CONFIG_TARGET_LIBS+= \
	configure-target-libsanitizer
.endif
.endif

.if ${HAVE_GCC} <= 10
EXTRA_FILES=	gcov-iov.h
.else
EXTRA_FILES=	bversion.h gtyp-input.list
.endif

BINENV=		/usr/bin/env -i


MKNATIVE_ENV=	${BINENV} ${CONFIGURE_ENV:NC*:NLD*} \
			CC_FOR_BUILD=${CC_FOR_BUILD:Q} \
			CXX_FOR_BUILD=${HOST_CXX:Q} \
			CFLAGS_FOR_BUILD="-I${TOOLDIR}/include" \
			CC=${CC:Q}' '${CCADDFLAGS:Q} \
			CXX=${CXX:Q}' '${CCADDFLAGS:Q}' '${CXXADDFLAGS:Q} \
			CPP=${CPP:Q}' '-I${DESTDIR}/usr/include' '-I${DESTDIR}/usr/include/g++/bits \
			CFLAGS= CPPFLAGS= CXXFLAGS= LDFLAGS= \
			AS=${AS:Q} AWK=${TOOL_AWK:Q} LD=${LD:Q} \
			MSGFMT=${TOOLDIR}/bin/${_TOOL_PREFIX}msgfmt \
			NM=${NM:Q} OBJDUMP=${OBJDUMP:Q} \
			XGETTEXT=${TOOLDIR}/bin/${_TOOL_PREFIX}xgettext \
			ac_cv_prog_cc_cross=yes \
			ac_cv_func_strcoll_works=yes \
			ac_cv_func_elf_getshstrndx=no \
			gcc_cv_func_printf_ptr=yes \
			gcc_cv_libc_provides_ssp=yes \
			gdb_cv_printf_has_long_double=yes \
			gdb_cv_printf_has_long_long=yes \
			gdb_cv_scanf_has_long_double=yes \
			gcc_cv_as_sparc_gotdata_op=no

CXXADDFLAGS=	--sysroot=${DESTDIR}
CCADDFLAGS=	--sysroot=${DESTDIR} -L${DESTDIR}/lib -L${DESTDIR}/usr/lib -B${DESTDIR}/usr/lib/ -I${.OBJDIR}/.native/gcc/include

# NEWCONFIGDIR can be set to a read-write location of the source tree
# in case the version being used is not.
NEWCONFIGDIR?=	${.CURDIR}/../..

GCC_MACHINE_ARCH=	${MACHINE_ARCH:S/earmv5/earm/}

bootstrap-libgcc: .configure_done
	@echo 'Creating files needed for libgcc by a native bootstrap build.'
	(cd ${.OBJDIR}/build && ${BUILD_COMMAND} configure-target-libgcc)
	@MAKE=${BUILD_MAKE:Q} ${HOST_SH} ${MKNATIVE} libgcc-bootstrap \
		${.OBJDIR}/build ${NEWCONFIGDIR} ${NETBSDSRCDIR} \
		${MACHINE_GNU_PLATFORM} ${GCC_MACHINE_ARCH} ${DESTDIR} ${TOOLDIR}

bootstrap-libstdc++: .configure_done
	@echo 'Creating files needed for libstdc++ by a native bootstrap build.'
	(cd ${.OBJDIR}/build && ${BUILD_COMMAND} configure-target-libstdc++-v3)
	@MAKE=${BUILD_MAKE:Q} ${HOST_SH} ${MKNATIVE} libstdc++-bootstrap \
		${.OBJDIR}/build ${NEWCONFIGDIR} ${NETBSDSRCDIR} \
		${MACHINE_GNU_PLATFORM} ${GCC_MACHINE_ARCH} ${DESTDIR} ${TOOLDIR}

native-gcc: .native/.configure_done
	@echo 'Extracting GNU GCC configury for a native toolchain.'
	@MAKE=${BUILD_MAKE:Q} ${HOST_SH} ${MKNATIVE} gcc \
		${.OBJDIR}/.native ${NEWCONFIGDIR} ${NETBSDSRCDIR} \
		${MACHINE_GNU_PLATFORM} ${GCC_MACHINE_ARCH} ${DESTDIR} ${TOOLDIR}

NATIVE_CONFIGURE_ARGS=	${COMMON_CONFIGURE_ARGS}
.include "lgpl3.mk"

.if ${MACHINE_ARCH} != "vax"
NATIVE_CONFIGURE_ARGS+= --enable-tls
.endif
.if !empty(MACHINE_ARCH:Mearm*)
NATIVE_CONFIGURE_ARGS+=	--enable-initfini-array
.endif

.if ${MACHINE_ARCH} == "m68000"
NATIVE_CONFIGURE_ARGS+= --disable-shared
.endif

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
EXTRA_GCC_TARGETS=	i386-builtin-types.inc
.endif
.if !empty(MACHINE_ARCH:M*arm*)
EXTRA_GCC_TARGETS=	arm-cpu-data.h arm-cpu-cdata.h
.endif

NATIVE_CONFIGURE_ARGS+=	\
			--disable-multilib \
			--disable-libstdcxx-pch \
			--build=`${GCCDIST}/config.guess` \
			--host=${MACHINE_GNU_PLATFORM} \
			--with-sysroot=${DESTDIR}
MKENV_BUILD_MAKE=cd .native && ${MKNATIVE_ENV} ${BUILD_MAKE}

.native/.configure_done: ${_GNU_CFGSRC} ${.CURDIR}/Makefile
	mkdir .native .native/gcc .native/gcc/include 2>/dev/null || true
	# If this fails, gthreads won't be configured initially, but reconfig will.
	cp ${DESTDIR}/usr/include/g++/bits/gthr-default.h .native/gcc/include || true
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native && ${MKNATIVE_ENV} \
			${HOST_SH} ${GNUHOSTDIST}/configure \
			${NATIVE_CONFIGURE_ARGS}) && \
		(${MKENV_BUILD_MAKE} all-build-libiberty) && \
		((${MKENV_BUILD_MAKE} all-libcpp) && \
		 (${MKENV_BUILD_MAKE} all-libbacktrace) && \
		 (${MKENV_BUILD_MAKE} all-libdecnumber) || true)
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(${MKENV_BUILD_MAKE} configure-gcc configure-libcpp) && \
		(${MKENV_BUILD_MAKE} configure-libiberty) && \
		(${MKENV_BUILD_MAKE} configure-libdecnumber)
	# edit Makefile so that maybe-all-gcc does not depend on all-gcc any more.
		(cd .native && mv Makefile Makefile.config && \
			${TOOL_SED} -e 's/\(maybe-all-gcc:\) all-gcc/\1/' \
				    -e 's/\(maybe-all-target-libgcc:\) all-target-libgcc/\1/' \
				< Makefile.config > Makefile)
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native/gcc && ${MKNATIVE_ENV} ${BUILD_MAKE} -e tree-check.h config.h multilib.h ${EXTRA_FILES})
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native/gcc && ${MKNATIVE_ENV} ${BUILD_MAKE} -e libgcc.mvars tconfig.h); \
		(cd .native && touch gcc/cc1obj gcc/cc1plus gcc/f771 gcc/libgcc.a gcc/libgcc_s.so)
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native/gcc && ${MKNATIVE_ENV} ${BUILD_MAKE} -e s-omp-device-properties-h)
.if defined(EXTRA_GCC_TARGETS)
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native/gcc && ${MKNATIVE_ENV} ${BUILD_MAKE} -e ${EXTRA_GCC_TARGETS})
.endif
.for _lib in ${MKNATIVE_CONFIG_TARGET_LIBS}
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native && ${MKNATIVE_ENV} ${BUILD_MAKE} \
			${_lib} \
			ALL_GCC_C= ALL_GCC_CXX= \
			CC_FOR_TARGET=${CC:Q}' '${CCADDFLAGS:Q} \
			CXX_FOR_TARGET=${CXX:Q}' '${CCADDFLAGS:Q}' '${CXXADDFLAGS:Q} \
			RAW_CXX_FOR_TARGET=${CXX:Q}' '${CCADDFLAGS:Q}' '${CXXADDFLAGS:Q} \
			CPP=${CPP:Q}' '-I${DESTDIR}/usr/include \
			ac_cv_prog_cc_cross=yes) && \
	true
.endfor
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native/${MACHINE_GNU_PLATFORM}/libstdc++-v3/include && \
			${MKNATIVE_ENV} ${BUILD_MAKE} \
			CC_FOR_TARGET=${CC:Q}' '${CCADDFLAGS:Q} \
			CXX_FOR_TARGET=${CXX:Q}' '${CCADDFLAGS:Q}' '${CXXADDFLAGS:Q} \
			CPP=${CPP:Q}' '-I${DESTDIR}/usr/include \
			all-local) && \
	true
	@touch $@

.if !defined(PREV_GCC)
.  if exists(PREV_GCC)
PREV_GCC!=	cat PREV_GCC
.  else
PREV_GCC=
.  endif
.endif

CLEANFILES+=	PREV_GCC

# Include the local NetBSD GCC version so that bumps to it trigger rebuilds.
NETBSD_GCC_VERSION_NOSPACE= ${NETBSD_GCC_VERSION:S/ /-/gW}

realall realdepend: .MAKE
.if !empty(PREV_GCC) && "${PREV_GCC}" != "${EXTERNAL_GCC_SUBDIR}-${HAVE_GCC}-${NETBSD_GCC_VERSION_NOSPACE}"
	@echo "*** WARNING: GCC has changed version?"
	@echo "*** PREV_GCC '${PREV_GCC}'"
	@echo "*** NETBSD_GCC_VERSION_NOSPACE '${NETBSD_GCC_VERSION_NOSPACE}'"
	@echo "***     !=  EXTERNAL_GCC_SUBDIR-HAVE_GCC-NETBSD_GCC_VERSION_NOSPACE:"
	@echo "***     !=  '${EXTERNAL_GCC_SUBDIR}-${HAVE_GCC}-${NETBSD_GCC_VERSION_NOSPACE}'"
	@echo "*** Cleaning mis-matched tools/gcc"
	rm -f PREV_GCC
	(cd ${.CURDIR} && ${MAKE} cleandir)
.endif
	echo ${EXTERNAL_GCC_SUBDIR}-${HAVE_GCC}-${NETBSD_GCC_VERSION_NOSPACE} >PREV_GCC

clean: clean.native
clean.native:
	-rm -r -f .native
