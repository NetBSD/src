# $NetBSD: Makefile,v 1.1 2008/05/01 15:33:33 jmmv Exp $

NOLINT=		# defined

.include <bsd.own.mk>

LIB=		atf-c
NOPIC=		# defined; interface is still too unstable

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}
.PATH:		${SRCDIR}/atf-c

CPPFLAGS+=	-I${.CURDIR}
CPPFLAGS+=	-I.

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-DATF_ARCH=\"${MACHINE}\"
CPPFLAGS+=	-DATF_CONFDIR=\"/etc/atf\"
CPPFLAGS+=	-DATF_LIBEXECDIR=\"/usr/libexec\"
CPPFLAGS+=	-DATF_MACHINE=\"${MACHINE_ARCH}\"
CPPFLAGS+=	-DATF_PKGDATADIR=\"/usr/share/atf\"
CPPFLAGS+=	-DATF_SHELL=\"/bin/sh\"
CPPFLAGS+=	-DATF_WORKDIR=\"/tmp\"

WARNS?=		2

SRCS=		config.c \
		dynstr.c \
		env.c \
		error.c \
		expand.c \
		fs.c \
		io.c \
		list.c \
		map.c \
		object.c \
		sanity.c \
		signals.c \
		text.c \
		ui.c \
		user.c \
		tc.c \
		tcr.c \
		tp.c \
		tp_main.c

INCS=		config.h \
		defs.h \
		dynstr.h \
		env.h \
		error.h \
		expand.h \
		fs.h \
		io.h \
		list.h \
		macros.h \
		map.h \
		object.h \
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

defs.h: defs.h.in
	sed -e 's,@ATTRIBUTE_NORETURN@,__attribute__((noreturn)),g' \
		<${SRCDIR}/atf-c/defs.h.in >defs.h.tmp
	mv defs.h.tmp defs.h

.include <bsd.lib.mk>
