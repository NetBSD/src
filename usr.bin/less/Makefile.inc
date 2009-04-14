#	$NetBSD: Makefile.inc,v 1.4 2009/04/14 22:15:22 lukem Exp $

WARNS?=	1	# many -Wshadow -Wcast-qual & other issues

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

CWARNFLAGS+=	-Wno-strict-prototypes -Wno-missing-prototypes
