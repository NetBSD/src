# $NetBSD: Makefile.inc,v 1.15 2024/08/14 23:54:49 christos Exp $

USE_LIBEDIT=no
USE_TUI=yes
BINDIR=/usr/bin
WARNS?=		0
CPPFLAGS+=	-D_KERNTYPES
GMPDIR=		${NETBSDSRCDIR}/external/lgpl3/gmp/lib/libgmp
MPFRDIR=	${NETBSDSRCDIR}/external/lgpl3/mpfr

.include "Makefile.gdb_arch"

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-format-security \
			-Wno-tautological-compare -Wno-format-extra-args \
			-Wno-string-plus-int

COPTS+=			-fcommon

DIST:=		${.PARSEDIR}/dist

# Reduce debugging for these extremely large objects.
.if ${MKDEBUG:Uno} != "no"
CXXFLAGS+=      -g1
.endif
