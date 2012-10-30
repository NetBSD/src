# $NetBSD: Makefile.inc,v 1.3.2.2 2012/10/30 18:57:55 yamt Exp $

USE_LIBEDIT=no
USE_TUI=yes
BINDIR=/usr/bin
WARNS?=		0

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-format-security \
			-Wno-tautological-compare -Wno-format-extra-args \
			-Wno-string-plus-int

DIST=		${NETBSDSRCDIR}/external/gpl3/gdb/dist
