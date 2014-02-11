# $NetBSD: Makefile,v 1.4 2014/02/11 17:28:21 jmmv Exp $

NOLINT=		# defined

.include <bsd.init.mk>

LIB=		tools
LIBISCXX=	yes
LIBISPRIVATE=	yes

LIBDPLIBS+=     m	${.CURDIR}/../../../../../lib/libm

.PATH:		${SRCDIR}
.PATH:		${SRCDIR}/tools

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
CPPFLAGS+=	-I${SRCDIR}/tools
CPPFLAGS+=	-I.

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

.include <bsd.lib.mk>
