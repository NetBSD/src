# $NetBSD: Makefile.inc,v 1.5 2012/04/04 10:59:45 joerg Exp $

USE_LIBEDIT=no
USE_TUI=yes
BINDIR=/usr/bin
WARNS?=		0

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-conversion \
			-Wno-switch-enum -Wno-parentheses -Wno-comment \
			-Wno-format-security \
			-Wno-tautological-compare -Wno-format-extra-args \
			-Wno-string-plus-int

.if ${MACHINE} == "vax"
CWARNFLAGS.gcc+=	-Wno-overflow
.endif

DIST=		${NETBSDSRCDIR}/external/gpl3/gdb/dist
