#	$NetBSD: Makefile.inc,v 1.11 2020/09/06 07:20:32 mrg Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare
CWARNFLAGS.clang+=	-Wno-tautological-compare
CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member
CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

CPPFLAGS+= -D_OPENBSD_SOURCE

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client *and* setuid
