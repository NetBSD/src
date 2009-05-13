#	$NetBSD: Makefile.inc,v 1.3.44.1 2009/05/13 19:19:55 jym Exp $

WARNS?=	1	# many -Wshadow -Wcast-qual & other issues

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

CWARNFLAGS+=	-Wno-strict-prototypes -Wno-missing-prototypes
