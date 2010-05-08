# $NetBSD: Makefile,v 1.5 2010/05/08 16:57:24 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-run
.PATH:		${SRCDIR}/data ${SRCDIR}/atf-sh

BINDIR=		/usr/bin
PROG_CXX=	atf-run
SRCS=		atf-run.cpp config.cpp requirements.cpp test-program.cpp \
		timer.cpp
MAN=		atf-run.1

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${.CURDIR}/../../lib/libatf-c
LDADD+=		-latf-c++ -latf-c

COPTS.atf-run.cpp+=	-Wno-stack-protector

WARNS?=		2

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/atf
FILES=		atf-run.hooks
.endif

# Files in ${SRCDIR}/data.
FILES+=			atf-c.pc \
			atf-c++.pc
FILESDIR_atf-c.pc=	/usr/lib/pkgconfig
FILESDIR_atf-c++.pc=	/usr/lib/pkgconfig

realall: atf-c.pc
atf-c.pc: Makefile atf-c.pc.in
	${TOOL_SED} -e 's,__ATF_VERSION__,0.8,g' \
	    -e 's,__CC__,gcc,g' \
	    -e 's,__INCLUDEDIR__,/usr/include,g' \
	    -e 's,__LIBDIR__,/usr/lib,g' \
	    <${SRCDIR}/data/atf-c.pc.in >atf-c.pc
CLEANFILES+=	atf-c.pc

realall: atf-c++.pc
atf-c++.pc: Makefile atf-c++.pc.in
	${TOOL_SED} -e 's,__ATF_VERSION__,0.8,g' \
	    -e 's,__CXX__,g++,g' \
	    -e 's,__INCLUDEDIR__,/usr/include,g' \
	    -e 's,__LIBDIR__,/usr/lib,g' \
	    <${SRCDIR}/data/atf-c++.pc.in >atf-c++.pc
CLEANFILES+=	atf-c++.pc

.include <bsd.prog.mk>
