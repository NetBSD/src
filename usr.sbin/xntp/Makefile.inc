#	$NetBSD: Makefile.inc,v 1.7 1998/03/06 18:17:16 christos Exp $

WARNS?=	0
CPPFLAGS += -I${.CURDIR}/../include -DHAVE_CONFIG_H
LDADD += -lntp
DPADD += ${LIBNTP}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
