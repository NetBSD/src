# $NetBSD: Makefile,v 1.2 2014/02/11 16:11:28 jmmv Exp $

NOLINT=		# defined

.include <bsd.init.mk>

LIB=		tools
LIBISCXX=	yes
LIBISPRIVATE=	yes

LIBDPLIBS+=     m	${.CURDIR}/../../../../../lib/libm

.PATH:		${SRCDIR}
.PATH:		${SRCDIR}/tools

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-DATF_ARCH=\"${MACHINE_ARCH}\"
CPPFLAGS+=	-DATF_BUILD_CC=\"/usr/bin/cc\"
CPPFLAGS+=	-DATF_BUILD_CFLAGS=\"\"
CPPFLAGS+=	-DATF_BUILD_CPP=\"/usr/bin/cpp\"
CPPFLAGS+=	-DATF_BUILD_CPPFLAGS=\"\"
CPPFLAGS+=	-DATF_BUILD_CXX=\"/usr/bin/c++\"
CPPFLAGS+=	-DATF_BUILD_CXXFLAGS=\"\"
CPPFLAGS+=	-DATF_CONFDIR=\"/etc/atf\"
CPPFLAGS+=	-DATF_INCLUDEDIR=\"/usr/include\"
CPPFLAGS+=	-DATF_LIBDIR=\"/usr/lib\"
CPPFLAGS+=	-DATF_LIBEXECDIR=\"/usr/libexec\"
CPPFLAGS+=	-DATF_MACHINE=\"${MACHINE}\"
CPPFLAGS+=	-DATF_PKGDATADIR=\"/usr/share/atf\"
CPPFLAGS+=	-DATF_SHELL=\"/bin/sh\"
CPPFLAGS+=	-DATF_WORKDIR=\"/tmp\"
CPPFLAGS+=	-DLAST_SIGNO=63
CPPFLAGS+=	-I${SRCDIR}/tools
CPPFLAGS+=	-I.
CPPFLAGS+=	-I${TOPDIR}/lib/libatf-c  # For bconfig.h

WARNS?=		2

SRCS=		application.cpp \
		atffile.cpp \
		config.cpp \
		config_file.cpp \
		env.cpp \
		exceptions.cpp \
		expand.cpp \
		fs.cpp \
		io.cpp \
		parser.cpp \
		process.cpp \
		reader.cpp \
		requirements.cpp \
		signals.cpp \
		test-program.cpp \
		text.cpp \
		timers.cpp \
		ui.cpp \
		user.cpp

CLEANFILES+=	defs.hpp

beforedepend: defs.hpp
defs.hpp: defs.hpp.in
	${TOOL_SED} \
	    -e 's|@ATTRIBUTE_FORMAT_PRINTF@|__attribute__((__format__(__printf__, a, b)))|g' \
	    -e 's|@ATTRIBUTE_NORETURN@|__attribute__((__noreturn__))|g' \
	    -e 's|@ATTRIBUTE_UNUSED@|__attribute__((__unused__))|g' \
	    <${.ALLSRC} >${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

.include <bsd.lib.mk>
