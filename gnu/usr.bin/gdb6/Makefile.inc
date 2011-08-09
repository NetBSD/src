# $NetBSD: Makefile.inc,v 1.3 2011/08/09 13:02:11 joerg Exp $

USE_LIBEDIT=no
USE_TUI=yes
WARNS?=		0

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-tautological-compare -Wno-format-extra-args
