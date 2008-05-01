# $NetBSD: Makefile,v 1.1 2008/05/01 15:33:33 jmmv Exp $

NOLINT=		# defined

.include <bsd.own.mk>

LIB=		atf-c++
LDADD=		-L${.OBJDIR}/../libatf-c -latf-c
NOPIC=		# defined; interface is still too unstable

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}
.PATH:		${SRCDIR}/atf-c++

CPPFLAGS+=	-I${.CURDIR}/../libatf-c
CPPFLAGS+=	-I.

CPPFLAGS+=	-DHAVE_CONFIG_H

WARNS?=		2

SRCS=		application.cpp \
		atffile.cpp \
		config.cpp \
		env.cpp \
		exceptions.cpp \
		expand.cpp \
		formats.cpp \
		fs.cpp \
		io.cpp \
		parser.cpp \
		signals.cpp \
		tests.cpp \
		text.cpp \
		ui.cpp \
		user.cpp

INCS=		application.hpp \
		atffile.hpp \
		config.hpp \
		env.hpp \
		exceptions.hpp \
		expand.hpp \
		formats.hpp \
		fs.hpp \
		io.hpp \
		macros.hpp \
		parser.hpp \
		sanity.hpp \
		signals.hpp \
		tests.hpp \
		text.hpp \
		ui.hpp \
		user.hpp \
		utils.hpp
INCSDIR=	/usr/include/atf-c++

INCS+=		atf-c++.hpp
INCSDIR_atf-c++.hpp=/usr/include

MAN=		# empty

.include <bsd.lib.mk>
