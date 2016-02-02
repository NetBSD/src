# $NetBSD: Makefile.inc,v 1.5 2016/02/02 20:57:28 christos Exp $

USE_LIBEDIT=no
USE_TUI=yes
BINDIR=/usr/bin
WARNS?=		0
CPPFLAGS+=	-D_KERNTYPES

GDB_MACHINE_ARCH=	${MACHINE_ARCH:C/armv[4-7]/arm/}

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-format-security \
			-Wno-tautological-compare -Wno-format-extra-args \
			-Wno-string-plus-int

DIST:=		${.PARSEDIR}/dist
