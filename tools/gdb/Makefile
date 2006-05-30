#	$NetBSD: Makefile,v 1.3 2006/05/30 04:22:40 nathanw Exp $

.include <bsd.own.mk>

MODULE=		gdb6

FIND_ARGS=	\! \( -type d -name sim -prune \)

CONFIGURE_ARGS=	--target=${MACHINE_GNU_PLATFORM} --disable-nls \
		--program-transform-name="s,^,${MACHINE_GNU_PLATFORM}-,"

MAKE_ARGS=	MACHINE= MAKEINFO=${TOOL_MAKEINFO:Q}

ALL_TARGET=	all-gdb
INSTALL_TARGET=	install-gdb

.include "${.CURDIR}/../Makefile.gnuhost"

CCADDFLAGS=	-I${DESTDIR}/usr/include -L${DESTDIR}/lib -L${DESTDIR}/usr/lib -B${DESTDIR}/usr/lib/

NEWCONFIGDIR?=	${.CURDIR}/../..
MKNATIVE?=	${.CURDIR}/mknative-gdb

native-gdb: .native/.configure_done
	@echo 'Extracting GDB configury for a native toolchain.'
	MAKE=${MAKE:Q} ${HOST_SH} ${MKNATIVE} gdb \
		${.OBJDIR}/.native ${NEWCONFIGDIR} ${MACHINE_GNU_PLATFORM}

.native/.configure_done: ${_GNU_CFGSRC} ${.CURDIR}/Makefile
	mkdir ${.OBJDIR}/.native 2>/dev/null || true
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd ${.OBJDIR}/.native && ${CONFIGURE_ENV:NC*:NLD*} \
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
			${HOST_SH} ${GNUHOSTDIST}/configure \
			--build=`${GNUHOSTDIST}/config.guess` \
			--host=${MACHINE_GNU_PLATFORM} \
			--target=${MACHINE_GNU_PLATFORM}) && \
		(cd ${.OBJDIR}/.native && ${MAKE} configure-host) && \
		(cd ${.OBJDIR}/.native/bfd && ${MAKE} bfd.h bfdver.h) && \
		(cd ${.OBJDIR}/.native/gdb && ${MAKE} init.c version.c)
	@touch $@

clean: clean.native
clean.native:
	-rm -r -f .native
