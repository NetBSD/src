# $NetBSD: Makefile,v 1.8 2010/11/07 17:46:46 jmmv Exp $

NOLINT=		# defined

.include <bsd.own.mk>

LIB=		atf-c
NOPIC=		# defined; interface is still too unstable

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}
.PATH:		${SRCDIR}/atf-c
.PATH:		${SRCDIR}/atf-c/detail

CPPFLAGS+=	-I${.CURDIR}
CPPFLAGS+=	-I.

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-DATF_ARCH=\"${MACHINE}\"
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
CPPFLAGS+=	-DATF_MACHINE=\"${MACHINE_ARCH}\"
CPPFLAGS+=	-DATF_M4=\"/usr/bin/m4\"
CPPFLAGS+=	-DATF_PKGDATADIR=\"/usr/share/atf\"
CPPFLAGS+=	-DATF_SHELL=\"/bin/sh\"
CPPFLAGS+=	-DATF_WORKDIR=\"/tmp\"

WARNS?=		4

SRCS=		build.c \
		check.c \
		config.c \
		dynstr.c \
		env.c \
		error.c \
		fs.c \
		list.c \
		map.c \
		process.c \
		sanity.c \
		text.c \
		user.c \
		utils.c \
		tc.c \
		tp.c \
		tp_main.c

INCS=		build.h \
		check.h \
		config.h \
		defs.h \
		error.h \
		error_fwd.h \
		macros.h \
		tc.h \
		tp.h \
		utils.h
INCSDIR=	/usr/include/atf-c

INCS+=		atf-c.h
INCSDIR_atf-c.h=/usr/include

MAN=		atf-c-api.3

CLEANFILES+=	defs.h

defs.h: defs.h.in
	${TOOL_SED} -e \
	    's,@ATTRIBUTE_NORETURN@,__attribute__((__noreturn__)),g' \
	    < ${.ALLSRC} > ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

.if ${MKSHARE} != "no"
FILES+=		atf-c.pc
FILESDIR=	/usr/lib/pkgconfig

realall: atf-c.pc
atf-c.pc: Makefile atf-c.pc.in
	${TOOL_SED} -e 's,__ATF_VERSION__,0.12,g' \
	    -e 's,__CC__,gcc,g' \
	    -e 's,__INCLUDEDIR__,/usr/include,g' \
	    -e 's,__LIBDIR__,/usr/lib,g' \
	    <${SRCDIR}/atf-c/atf-c.pc.in >atf-c.pc
CLEANFILES+=	atf-c.pc
.endif

.include <bsd.lib.mk>
