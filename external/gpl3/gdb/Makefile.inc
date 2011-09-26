# $NetBSD: Makefile.inc,v 1.1 2011/09/26 03:02:44 christos Exp $

USE_LIBEDIT=no
USE_TUI=yes
WARNS?=		0

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-tautological-compare -Wno-format-extra-args

DIST=		${NETBSDSRCDIR}/external/gpl3/gdb/dist
