#	$NetBSD: Makefile,v 1.41 2025/09/24 04:43:01 rillig Exp $

.include <bsd.hostinit.mk>

MODULE=		binutils

GNUHOSTDIST=	${.CURDIR}/../../external/gpl3/${EXTERNAL_BINUTILS_SUBDIR}/dist

BRANDING?=	\
	--with-pkgversion="NetBSD Binutils nb1" \
	--with-bugurl="http://www.NetBSD.org/support/send-pr.html" \
	--with-lib-path="=/usr/lib" --with-sysroot

BASE_CONFIGURE_ARGS= \
	--target=${MACHINE_GNU_PLATFORM} \
	--enable-initfini-array=yes \
	--enable-default-execstack=no \
	${BRANDING}

CONFIGURE_ARGS=	--disable-nls --disable-werror \
		--with-zstd=no \
		--program-transform-name="s,^,${MACHINE_GNU_PLATFORM}-," \
		${BASE_CONFIGURE_ARGS}

MAKE_ARGS=	MACHINE= MAKEINFO=${TOOL_MAKEINFO:Q}

ALL_TARGET=	all-binutils all-gas all-ld
INSTALL_TARGET=	install-binutils install-gas install-ld
.if ${MKCROSSGPROF:Uno} != "no"
ALL_TARGET+=	all-gprof
INSTALL_TARGET+=install-gprof
.endif

.include "${.CURDIR}/../Makefile.gmakehost"

CCADDFLAGS=	-I${GNUHOSTDIST}/include  -I${DESTDIR}/usr/include
CCADDFLAGS+=	-B${DESTDIR}/usr/lib/
CCADDFLAGS+=	${HOST_CPPFLAGS}
LDADDFLAGS=	-L${DESTDIR}/lib -L${DESTDIR}/usr/lib

# Force avoiding possibly non-executable install-sh.
CONFIGURE_ENV+= ac_cv_path_mkdir="${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install -d"

NEWCONFIGDIR?=	${.CURDIR}/../..
.if ${EXTERNAL_BINUTILS_SUBDIR} == "binutils.old"
MKNATIVE=	${.CURDIR}/mknative-binutils.old
.endif
MKNATIVE?=	${.CURDIR}/mknative-binutils

native-binutils: .native/.configure_done
	@echo 'Extracting GNU binutils configury for a native toolchain.'
	MAKE=${MAKE_PROGRAM:Q} ${HOST_SH} ${MKNATIVE} binutils \
		${.OBJDIR}/.native ${NEWCONFIGDIR} ${MACHINE_GNU_PLATFORM}

.native/.configure_done: ${_GNU_CFGSRC} ${.CURDIR}/Makefile
	mkdir .native 2>/dev/null || true
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native && ${CONFIGURE_ENV:NC*:NLD*} \
			CC_FOR_BUILD=${HOST_CC:Q} \
			CC=${CC:Q}' '${CCADDFLAGS:Q} \
			CXX=${CXX:Q}' '${CCADDFLAGS:Q} \
			CPP=${CPP:Q}' '-I${DESTDIR}/usr/include \
			CFLAGS= CPPFLAGS= CXXFLAGS= \
			LDFLAGS=${LDADDFLAGS:Q} \
			MSGFMT=${TOOLDIR}/bin/${_TOOL_PREFIX}msgfmt \
			XGETTEXT=${TOOLDIR}/bin/${_TOOL_PREFIX}xgettext \
			LIBS=-lintl \
			ac_cv_prog_cc_cross=yes \
			ac_cv_func_strcoll_works=yes \
			${HOST_SH} ${GNUHOSTDIST}/configure \
			--build=`${GNUHOSTDIST}/config.guess` \
			--host=${MACHINE_GNU_PLATFORM} \
			${BASE_CONFIGURE_ARGS} \
		)
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native && ${MAKE} configure-host)
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native/bfd && ${MAKE} bfd.h bfdver.h)
	PATH=${TOOLDIR}/bin:$$PATH; export PATH; \
		(cd .native/ld && ${MAKE} ldemul-list.h)
	@touch $@

CLEANFILES+=	PREV_BINUTILS

.if !defined(PREV_BINUTILS)
.  if exists(PREV_BINUTILS)
PREV_BINUTILS!=	cat PREV_BINUTILS
.  else
PREV_BINUTILS=
.  endif
.endif

CLEANFILES+=	PREV_BINUTILS

# Include the local NetBSD BINUTILS version so that bumps to it trigger rebuilds.
NETBSD_BINUTILS_VERSION_NOSPACE=nb1	# just to keep it the same with gcc

realall realdepend: .MAKE
.if !empty(PREV_BINUTILS) && "${PREV_BINUTILS}" != "${EXTERNAL_BINUTILS_SUBDIR}-${HAVE_BINUTILS}-${NETBSD_BINUTILS_VERSION_NOSPACE}"
	@echo "*** WARNING: BINUTILS has changed version?"
	@echo "*** PREV_BINUTILS '${PREV_BINUTILS}'"
	@echo "*** NETBSD_BINUTILS_VERSION_NOSPACE '${NETBSD_BINUTILS_VERSION_NOSPACE}'"
	@echo "***     !=  EXTERNAL_BINUTILS_SUBDIR-HAVE_BINUTILS-NETBSD_BINUTILS_VERSION_NOSPACE:"
	@echo "***     !=  '${EXTERNAL_BINUTILS_SUBDIR}-${HAVE_BINUTILS}-${NETBSD_BINUTILS_VERSION_NOSPACE}'"
	@echo "*** Cleaning mis-matched tools/binutils"
	rm -f PREV_BINUTILS
	(cd ${.CURDIR} && ${MAKE} cleandir)
.endif
	echo ${EXTERNAL_BINUTILS_SUBDIR}-${HAVE_BINUTILS}-${NETBSD_BINUTILS_VERSION_NOSPACE} >PREV_BINUTILS

clean: clean.native
clean.native:
	-rm -r -f .native

