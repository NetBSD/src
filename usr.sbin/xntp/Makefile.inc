#	$NetBSD: Makefile.inc,v 1.6 1997/10/25 06:58:54 lukem Exp $

WARNS?=	0
CPPFLAGS += -I${.CURDIR}/../include -DHAVE_CONFIG_H
LDADD += -lntp
DPADD += ${LIBNTP}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
