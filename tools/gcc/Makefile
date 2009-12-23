#	$NetBSD: Makefile,v 1.33 2009/12/23 20:15:21 mrg Exp $

.include <bsd.own.mk>

GCC_LANGUAGES=c c++ objc
MODULE=			gcc4
MKNATIVE_TARGET=	gcc4
MKNATIVE_CONFIG_TARGET_LIBS=
BINENV=		/usr/bin/env -i

VAX_CONFIGURE_ARGS=

.if ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "sparc64"
MULTILIB_ARGS= --enable-multilib
.else
MULTILIB_ARGS= --disable-multilib
.endif

COMMON_CONFIGURE_ARGS=	--target=${MACHINE_GNU_PLATFORM} \
			--enable-long-long \
			--enable-threads \
			${VAX_CONFIGURE_ARGS} \
			--enable-__cxa_atexit
.if defined(GCC_CONFIG_ARCH.${MACHINE_ARCH})
COMMON_CONFIGURE_ARGS+=	--with-arch=${GCC_CONFIG_ARCH.${MACHINE_ARCH}}
.endif
.if defined(GCC_CONFIG_TUNE.${MACHINE_ARCH})
COMMON_CONFIGURE_ARGS+=	--with-tune=${GCC_CONFIG_TUNE.${MACHINE_ARCH}}
.endif

CONFIGURE_ARGS=	${COMMON_CONFIGURE_ARGS} \
		--disable-nls \
		${MULTILIB_ARGS} \
		--program-transform-name="s,^,${MACHINE_GNU_PLATFORM}-," \
		--enable-languages="${GCC_LANGUAGES}"

MAKE_ARGS=	MACHINE= MAKEINFO=${TOOL_MAKEINFO:Q} \
		LIBGCC= LIBGCC1= LIBGCC1_TEST= LIBGCC2= INSTALL_LIBGCC= \
		EXTRA_PARTS= CPPFLAGS=-DNETBSD_TOOLS \
		AR=${HOST_AR:Q} RANLIB=${HOST_RANLIB:Q}

CONFIGURE_ENV+= gcc_cv_libc_provides_ssp=yes

MKNATIVE_ENV=	${BINENV} ${CONFIGURE_ENV:NC*:NLD*} \
			BOOTSTRAP_CFLAGS=-O2 \
			CC_FOR_BUILD=${HOST_CC:Q} \
			CC=${CC:Q}' '${CCADDFLAGS:Q} \
			CXX=${CXX:Q}' '${CCADDFLAGS:Q}' '${CXXADDFLAGS:Q} \
			CPP=${CPP:Q}' '-I${DESTDIR}/usr/include \
			CFLAGS= CPPFLAGS= CXXFLAGS= LDFLAGS= \
			AS=${AS:Q} AWK=${TOOL_AWK:Q} LD=${LD:Q} \
			MSGFMT=${TOOLDIR}/bin/${_TOOL_PREFIX}msgfmt \
			NM=${NM:Q} OBJDUMP=${OBJDUMP:Q} \
			XGETTEXT=${TOOLDIR}/bin/${_TOOL_PREFIX}xgettext \
			LIBS=-lintl \
			ac_cv_prog_cc_cross=yes \
			ac_cv_func_strcoll_works=yes \
			gcc_cv_func_printf_ptr=yes \
			gcc_cv_libc_provides_ssp=yes \
			gdb_cv_printf_has_long_double=yes \
			gdb_cv_printf_has_long_long=yes \
			gdb_cv_scanf_has_long_double=yes

ALL_TARGET=	all-gcc
INSTALL_TARGET=	install-gcc

.include "${.CURDIR}/../Makefile.gmakehost"
BUILD_MAKE=${TOOL_GMAKE}

CXXADDFLAGS=	${CPPFLAG_ISYSTEMXX} ${DESTDIR}/usr/include/g++
CCADDFLAGS=	${CPPFLAG_ISYSTEM} ${DESTDIR}/usr/include -L${DESTDIR}/lib -L${DESTDIR}/usr/lib -B${DESTDIR}/usr/lib/

NEWCONFIGDIR?=	${.CURDIR}/../..
MKNATIVE?=	${.CURDIR}/mknative-gcc

bootstrap-libgcc: .configure_done
	@echo 'Creating files needed for libgcc by a native bootstrap build.'
	@MAKE=${BUILD_MAKE:Q} ${HOST_SH} ${MKNATIVE} lib${MODULE} \
		${.OBJDIR}/build ${NEWCONFIGDIR} ${MACHINE_GNU_PLATFORM}

native-gcc: .native/.configure_done
	@echo 'Extracting GNU GCC configury for a native toolchain.'
	@MAKE=${BUILD_MAKE:Q} ${HOST_SH} ${MKNATIVE} ${MKNATIVE_TARGET} \
		${.OBJDIR}/.native ${NEWCONFIGDIR} ${MACHINE_GNU_PLATFORM}

NATIVE_CONFIGURE_ARGS=	${COMMON_CONFIGURE_ARGS} \
			--disable-multilib \
			--disable-symvers \
			--build=`${.CURDIR}/../../gnu/dist/${MODULE}/config.guess` \
			--host=${MACHINE_GNU_PLATFORM}

.native/.configure_done: ${_GNU_CFGSRC} ${.CURDIR}/Makefile
	mkdir .native 2>/dev/null || true
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native && ${MKNATIVE_ENV} \
			${HOST_SH} ${GNUHOSTDIST}/configure \
			${NATIVE_CONFIGURE_ARGS}) && \
		(cd .native && ${MKNATIVE_ENV} ${BUILD_MAKE} all-build-libiberty)
.if ${HAVE_GCC} == 4
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native && ${MKNATIVE_ENV} ${BUILD_MAKE} configure-gcc configure-libcpp) && \
		(cd .native && ${MKNATIVE_ENV} ${BUILD_MAKE} configure-libiberty)
	# edit Makefile so that maybe-all-gcc does not depend on all-gcc any more.
		(cd .native && mv Makefile Makefile.config && \
			${TOOL_SED} -e 's/\(maybe-all-gcc:\) all-gcc/\1/' \
				< Makefile.config > Makefile) && \
		(cd .native/gcc && ${MKNATIVE_ENV} ${BUILD_MAKE} -e tree-check.h config.h multilib.h gcov-iov.h)
.else
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native && ${MKNATIVE_ENV} ${BUILD_MAKE} all-libiberty) && \
		(cd .native/gcc && ${MKNATIVE_ENV} ${BUILD_MAKE} tree-check.h config.h)
.endif
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native && touch gcc/cc1obj gcc/cc1plus gcc/f771 gcc/libgcc.a gcc/libgcc_s.so && \
			${MKNATIVE_ENV} ${BUILD_MAKE} configure-target-libiberty \
			${MKNATIVE_CONFIG_TARGET_LIBS} configure-target-libobjc \
			configure-target-libstdc++-v3 \
			ALL_GCC_C= ALL_GCC_CXX= \
			CC_FOR_TARGET=${CC:Q}' '${CCADDFLAGS:Q} \
			CXX_FOR_TARGET=${CXX:Q}' '${CCADDFLAGS:Q}' '${CXXADDFLAGS:Q} \
			RAW_CXX_FOR_TARGET=${CXX:Q}' '${CCADDFLAGS:Q}' '${CXXADDFLAGS:Q} \
			CPP=${CPP:Q}' '-I${DESTDIR}/usr/include \
			ac_cv_prog_cc_cross=yes) && \
	true
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native/${MACHINE_GNU_PLATFORM}/libstdc++-v3/include && \
			${MKNATIVE_ENV} ${BUILD_MAKE} \
			CC_FOR_TARGET=${CC:Q}' '${CCADDFLAGS:Q} \
			CXX_FOR_TARGET=${CXX:Q}' '${CCADDFLAGS:Q}' '${CXXADDFLAGS:Q} \
			CPP=${CPP:Q}' '-I${DESTDIR}/usr/include \
			all-local) && \
	true
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native/${MACHINE_GNU_PLATFORM}/libiberty && \
			${MKNATIVE_ENV} ${BUILD_MAKE} needed-list)
	@touch $@

clean: clean.native
clean.native:
	-rm -r -f .native
