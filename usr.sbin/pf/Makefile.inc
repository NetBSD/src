#	$NetBSD: Makefile.inc,v 1.13 2023/06/03 21:26:29 lukem Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare
CWARNFLAGS.clang+=	-Wno-tautological-compare
CWARNFLAGS+=		${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

CPPFLAGS+= -D_OPENBSD_SOURCE

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client *and* setuid
