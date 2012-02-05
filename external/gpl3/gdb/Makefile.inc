# $NetBSD: Makefile.inc,v 1.4 2012/02/05 17:47:30 matt Exp $

USE_LIBEDIT=no
USE_TUI=yes
BINDIR=/usr/bin
WARNS?=		0

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-format-security \
			-Wno-tautological-compare -Wno-format-extra-args

.if ${MACHINE} == "vax"
CWARNFLAGS.gcc+=	-Wno-overflow
.endif

DIST=		${NETBSDSRCDIR}/external/gpl3/gdb/dist
