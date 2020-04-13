# $NetBSD: Makefile,v 1.5.30.2 2020/04/13 07:45:49 martin Exp $

NOLINT=		# defined

.include <bsd.init.mk>

LIB=		tools
LIBISCXX=	yes
LIBISPRIVATE=	yes

LIBDPLIBS+=     m	${.CURDIR}/../../../../../lib/libm

.PATH:		${SRCDIR}/tools

CPPFLAGS+=	-I${SRCDIR}/tools

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

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.parser.cpp += -Wno-error=maybe-uninitialized
.endif

.include <bsd.lib.mk>
