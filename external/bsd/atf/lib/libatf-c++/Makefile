# $NetBSD: Makefile,v 1.3 2010/06/04 08:33:41 jmmv Exp $

NOLINT=		# defined

.include <bsd.own.mk>

LIB=		atf-c++
LDADD=		-L${.OBJDIR}/../libatf-c -latf-c
NOPIC=		# defined; interface is still too unstable

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}
.PATH:		${SRCDIR}/atf-c++

CPPFLAGS+=	-I${.CURDIR}/../libatf-c
CPPFLAGS+=	-I.

CPPFLAGS+=	-DHAVE_CONFIG_H

WARNS?=		2

SRCS=		application.cpp \
		atffile.cpp \
		build.cpp \
		check.cpp \
		config.cpp \
		env.cpp \
		exceptions.cpp \
		expand.cpp \
		formats.cpp \
		fs.cpp \
		io.cpp \
		parser.cpp \
		process.cpp \
		signals.cpp \
		tests.cpp \
		text.cpp \
		ui.cpp \
		user.cpp

INCS=		application.hpp \
		atffile.hpp \
		build.hpp \
		check.hpp \
		config.hpp \
		env.hpp \
		exceptions.hpp \
		expand.hpp \
		formats.hpp \
		fs.hpp \
		io.hpp \
		macros.hpp \
		parser.hpp \
		process.hpp \
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

MAN=		atf-c++-api.3

.if ${MKSHARE} != "no"
FILES+=		atf-c++.pc
FILESDIR=	/usr/lib/pkgconfig

realall: atf-c++.pc
atf-c++.pc: Makefile atf-c++.pc.in
	${TOOL_SED} -e 's,__ATF_VERSION__,0.9,g' \
	    -e 's,__CXX__,g++,g' \
	    -e 's,__INCLUDEDIR__,/usr/include,g' \
	    -e 's,__LIBDIR__,/usr/lib,g' \
	    <${SRCDIR}/atf-c++/atf-c++.pc.in >atf-c++.pc
CLEANFILES+=	atf-c++.pc
.endif

.include <bsd.lib.mk>
