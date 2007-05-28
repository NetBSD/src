#	$NetBSD: Makefile.inc,v 1.5 2007/05/28 12:06:38 tls Exp $

CPPFLAGS+=	-include ${.CURDIR}/../compat_openbsd.h

WARNS?=	1

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client *and* setuid
