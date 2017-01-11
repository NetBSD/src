#	$NetBSD: Makefile.inc,v 1.10 2017/01/11 12:19:44 joerg Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare
CWARNFLAGS.clang+=	-Wno-tautological-compare
CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member

CPPFLAGS+= -D_OPENBSD_SOURCE

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client *and* setuid
