# $NetBSD: Makefile,v 1.3 2009/12/22 13:38:10 jmmv Exp $

NOLINT=		# defined

.include <bsd.own.mk>

LIB=		atf-c
NOPIC=		# defined; interface is still too unstable

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}
.PATH:		${SRCDIR}/atf-c

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
		expand.c \
		fs.c \
		io.c \
		list.c \
		map.c \
		object.c \
		process.c \
		sanity.c \
		signals.c \
		text.c \
		ui.c \
		user.c \
		tc.c \
		tcr.c \
		tp.c \
		tp_main.c

INCS=		build.h \
		check.h \
		config.h \
		defs.h \
		dynstr.h \
		env.h \
		error.h \
		error_fwd.h \
		expand.h \
		fs.h \
		io.h \
		list.h \
		macros.h \
		map.h \
		object.h \
		process.h \
		sanity.h \
		signals.h \
		tc.h \
		tcr.h \
		text.h \
		tp.h \
		ui.h \
		user.h
INCSDIR=	/usr/include/atf-c

INCS+=		atf-c.h
INCSDIR_atf-c.h=/usr/include

MAN=		# empty

CLEANFILES+=	defs.h

defs.h: defs.h.in
	${TOOL_SED} -e \
	    's,@ATTRIBUTE_NORETURN@,__attribute__((__noreturn__)),g' \
	    < ${.ALLSRC} > ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

.include <bsd.lib.mk>
