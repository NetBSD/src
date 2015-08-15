# $NetBSD: Makefile.inc,v 1.1.1.1 2015/08/15 09:27:49 christos Exp $

USE_LIBEDIT=no
USE_TUI=yes
BINDIR=/usr/bin
WARNS?=		0

GDB_MACHINE_ARCH=	${MACHINE_ARCH:C/armv[4-7]/arm/}

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-format-security \
			-Wno-tautological-compare -Wno-format-extra-args \
			-Wno-string-plus-int

DIST=		${NETBSDSRCDIR}/external/gpl3/gdb/dist
