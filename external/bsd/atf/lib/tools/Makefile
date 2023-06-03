# $NetBSD: Makefile,v 1.8 2023/06/03 21:31:45 lukem Exp $

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

COPTS.parser.cpp += ${CC_WNO_MAYBE_UNINITIALIZED}

.include <bsd.lib.mk>
