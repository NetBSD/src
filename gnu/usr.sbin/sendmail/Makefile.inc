#	$NetBSD: Makefile.inc,v 1.1.1.1 2000/02/08 13:45:34 tron Exp $

WARNS?=	0
CPPFLAGS+=	-DNETINET6 -DNETISO

DBMDEF=		-DNEWDB -DNIS

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
