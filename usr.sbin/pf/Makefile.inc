#	$NetBSD: Makefile.inc,v 1.7 2009/04/23 05:25:06 lukem Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

CPPFLAGS+=	-include ${.CURDIR}/../compat_openbsd.h

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client *and* setuid
