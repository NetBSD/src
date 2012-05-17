# $NetBSD: Makefile.inc,v 1.4.2.1 2012/05/17 18:02:31 riz Exp $

USE_LIBEDIT=no
USE_TUI=yes
BINDIR=/usr/bin
WARNS?=		0

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-format-security \
			-Wno-tautological-compare -Wno-format-extra-args

DIST=		${NETBSDSRCDIR}/external/gpl3/gdb/dist
