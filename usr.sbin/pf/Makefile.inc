#	$NetBSD: Makefile.inc,v 1.8 2011/05/26 12:56:34 joerg Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare
CWARNFLAGS.clang+=	-Wno-tautological-compare

CPPFLAGS+=	-include ${.CURDIR}/../compat_openbsd.h

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client *and* setuid
