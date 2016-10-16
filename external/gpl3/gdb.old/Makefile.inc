# $NetBSD: Makefile.inc,v 1.8 2016/10/16 04:36:51 mrg Exp $

USE_LIBEDIT=no
USE_TUI=yes
BINDIR=/usr/bin
WARNS?=		0
CPPFLAGS+=	-D_KERNTYPES

.include "Makefile.gdb_arch"

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-format-security \
			-Wno-tautological-compare -Wno-format-extra-args \
			-Wno-string-plus-int

DIST:=		${.PARSEDIR}/dist
