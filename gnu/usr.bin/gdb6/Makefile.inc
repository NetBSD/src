# $NetBSD: Makefile.inc,v 1.2 2011/05/26 12:56:28 joerg Exp $

USE_LIBEDIT=no
USE_TUI=yes
WARNS?=		0

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion -Wno-array-bounds \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-tautological-compare -Wno-format-extra-args
