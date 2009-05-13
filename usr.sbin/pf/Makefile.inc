#	$NetBSD: Makefile.inc,v 1.5.20.1 2009/05/13 19:20:32 jym Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

CPPFLAGS+=	-include ${.CURDIR}/../compat_openbsd.h

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client *and* setuid
