# $NetBSD: Makefile.inc,v 1.2 2011/10/09 13:58:27 christos Exp $

USE_LIBEDIT=no
USE_TUI=yes
BINDIR=/usr/bin
WARNS?=		0

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-tautological-compare -Wno-format-extra-args

DIST=		${NETBSDSRCDIR}/external/gpl3/gdb/dist
