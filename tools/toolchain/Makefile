#	$NetBSD: Makefile,v 1.26 2003/03/05 06:17:17 mrg Exp $

.include <bsd.own.mk>

MODULE=		toolchain

FIND_ARGS=	\! \( -type d -name sim -prune \)

CONFIGURE_ENV=	LANGUAGES="c c++ objc f77"
CONFIGURE_ARGS=	--target=${MACHINE_GNU_PLATFORM} --disable-nls \
		--program-transform-name="s,^,${MACHINE_GNU_PLATFORM}-,"

MAKE_ARGS=	MACHINE= MAKEINFO=${MAKEINFO:Q} \
		LIBGCC= LIBGCC1= LIBGCC1_TEST= LIBGCC2= INSTALL_LIBGCC=

ALL_TARGET=	all-binutils all-gas all-ld all-gcc
INSTALL_TARGET=	install-binutils install-gas install-ld install-gcc

.if ${MKCROSSGDB:Uno} == "yes"
ALL_TARGET+=	all-gdb
INSTALL_TARGET+=install-gdb
.endif

.include "${.CURDIR}/../Makefile.gnuhost"

CCADDFLAGS=	-I${DESTDIR}/usr/include -L${DESTDIR}/usr/lib -B${DESTDIR}/usr/lib/

NEWCONFIGDIR?=	${.CURDIR}/../..

bootstrap-libgcc: .configure_done
	@echo 'Creating files needed for libgcc by a native bootstrap build.'
	@MAKE=${MAKE:Q} sh ${.CURDIR}/mknative libgcc \
		${.OBJDIR}/build ${NEWCONFIGDIR} ${MACHINE_GNU_PLATFORM}

native: .native/.configure_done
	@echo 'Extracting GNU configury for a native toolchain.'
	@MAKE=${MAKE:Q} sh  ${.CURDIR}/mknative all \
		${.OBJDIR}/.native ${NEWCONFIGDIR} ${MACHINE_GNU_PLATFORM}

native-binutils: .native/.configure_done
	@echo 'Extracting GNU binutils configury for a native toolchain.'
	@MAKE=${MAKE:Q} sh  ${.CURDIR}/mknative binutils \
		${.OBJDIR}/.native ${NEWCONFIGDIR} ${MACHINE_GNU_PLATFORM}

native-gcc: .native/.configure_done
	@echo 'Extracting GNU GCC configury for a native toolchain.'
	@MAKE=${MAKE:Q} sh  ${.CURDIR}/mknative gcc \
		${.OBJDIR}/.native ${NEWCONFIGDIR} ${MACHINE_GNU_PLATFORM}

native-gdb: .native/.configure_done
	@echo 'Extracting GNU GDB configury for a native toolchain.'
	@MAKE=${MAKE:Q} sh  ${.CURDIR}/mknative gdb \
		${.OBJDIR}/.native ${NEWCONFIGDIR} ${MACHINE_GNU_PLATFORM}

.native/.configure_done: ${_GNU_CFGSRC} ${.CURDIR}/Makefile
	@mkdir .native 2>/dev/null || true
	@PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native && ${CONFIGURE_ENV:NC*:NLD*} \
			CC_FOR_BUILD=${HOST_CC:Q} \
			CC=${CC:Q}' '${CCADDFLAGS:Q} \
			CXX=${CXX:Q}' '${CCADDFLAGS:Q} \
			CPP=${CPP:Q}' '-I${DESTDIR}/usr/include \
			CFLAGS= CPPFLAGS= CXXFLAGS= LDFLAGS= \
			MSGFMT=${TOOLDIR}/bin/${_TOOL_PREFIX}msgfmt \
			XGETTEXT=${TOOLDIR}/bin/${_TOOL_PREFIX}xgettext \
			LIBS=-lintl \
			ac_cv_prog_cc_cross=yes \
			ac_cv_func_strcoll_works=yes \
			gcc_cv_func_printf_ptr=yes \
			gdb_cv_printf_has_long_double=yes \
			gdb_cv_printf_has_long_long=yes \
			gdb_cv_scanf_has_long_double=yes \
			sh ${GNUHOSTDIST}/configure \
			--build=`${.CURDIR}/../../gnu/dist/toolchain/config.guess` \
			--host=${MACHINE_GNU_PLATFORM} \
			--target=${MACHINE_GNU_PLATFORM}) && \
		(cd .native/bfd && ${MAKE} bfd.h) && \
		(cd .native/gcc && ${MAKE} tree-check.h HOST_CC=${HOST_CC:Q}) && \
		(cd .native/ld && ${MAKE} ldemul-list.h) && \
		(cd .native && touch gcc/cc1obj gcc/cc1plus gcc/f771 && \
			${MAKE} configure-target-libiberty \
			configure-target-libf2c configure-target-libobjc \
			configure-target-libio configure-target-libstdc++ \
			ALL_GCC_C= ALL_GCC_CXX= \
			CC_FOR_TARGET=${CC:Q}' '${CCADDFLAGS:Q} \
			CXX_FOR_TARGET=${CXX:Q}' '${CCADDFLAGS:Q} \
			CPP=${CPP:Q}' '-I${DESTDIR}/usr/include) && \
		(cd .native/${MACHINE_GNU_PLATFORM}/libio && \
			${MAKE} _G_config.h)
		(cd .native/${MACHINE_GNU_PLATFORM}/libiberty && \
			${MAKE} needed-list)
	@touch $@

clean: clean.native
clean.native:
	-rm -r -f .native
