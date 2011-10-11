# $NetBSD: Makefile.inc,v 1.3 2011/10/11 15:18:09 joerg Exp $

USE_LIBEDIT=no
USE_TUI=yes
BINDIR=/usr/bin
WARNS?=		0

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-format-security \
			-Wno-tautological-compare -Wno-format-extra-args

DIST=		${NETBSDSRCDIR}/external/gpl3/gdb/dist
