#	$NetBSD: Makefile.inc,v 1.9.2.1 2017/03/20 06:58:08 pgoyette Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare
CWARNFLAGS.clang+=	-Wno-tautological-compare
CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member

CPPFLAGS+= -D_OPENBSD_SOURCE

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client *and* setuid
