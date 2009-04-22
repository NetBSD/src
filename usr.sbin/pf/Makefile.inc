#	$NetBSD: Makefile.inc,v 1.6 2009/04/22 15:23:06 lukem Exp $

CPPFLAGS+=	-include ${.CURDIR}/../compat_openbsd.h

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client *and* setuid
