#	$NetBSD: Makefile,v 1.7 2026/10/18 23:04:41 tls Exp $
#

GNUHOSTDIST=   ${.CURDIR}/../../external/lgpl3/gmp/dist

CONFIGURE_ARGS+=--disable-shared

CPPFLAGS+=	${HOST_CPPFLAGS}

.include "${.CURDIR}/../Makefile.gnuhost"

# GMP uses 8-byte alignment for 16-byte align-requiring things that break
# the build on modern macOS targets, turn off assembly on x86-64 for now.
# https://trac.macports.org/ticket/70859
.if ${BUILD_OSTYPE} == "Darwin"
BUILD_MACHINE!=	uname -m
. if ${BUILD_MACHINE} == "x86_64"
CONFIGURE_ARGS+=	--disable-assembly
. endif
#
# GMP prior to 6.3 misuses register x18 on 64-bit ARM, causing
# mysterious compiler failures on Darwin where it is zeroed on
# context switch.  See GMP change 5f32dbc41afc if you want to
# fix this before upgrading GMP itself.
#
. if ${BUILD_MACHINE} == "arm64"
CONFIGURE_ARGS+=	--disable-assembly
. endif
.endif

# Don't override CFLAGS and CXXFLAGS, it breaks ABI detection.
# PATH needs special treating since "make" tries to expand $PATH
# when variable is used.
CONFIGURE_ENV:=	${CONFIGURE_ENV:NPATH=*:NCFLAGS=*:NCXXFLAGS=*} \
		PATH="${TOOLDIR:Q}/bin:$$PATH"

# Force avoiding possibly non-executable install-sh.
CONFIGURE_ENV+=	ac_cv_path_mkdir="${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install -d"

# mknative-gmp support
#
# XXX early version; does not use mknative.common yet.

native-gmp: .native/.configure_done
	@echo 'Extracting GNU GMP configury for a native toolchain.'
	(cd ${.OBJDIR}/.native && \
	 ${MAKE} -f ${.CURDIR}/../../external/lgpl3/gmp/Makefile.netbsd-gmp copy-files)
	@echo 'HA HA, only joking. Manual fixes maybe needed now.'

.native/.configure_done: ${_GNU_CFGSRC} ${.CURDIR}/Makefile
	mkdir .native 2>/dev/null || true
	(cd ${.OBJDIR}/.native && \
	 ${MAKE} -f ${.CURDIR}/../../external/lgpl3/gmp/Makefile.netbsd-gmp all)
