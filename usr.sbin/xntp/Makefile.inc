#	$NetBSD: Makefile.inc,v 1.5 1997/10/17 14:34:38 lukem Exp $

WARNS?=	0
CFLAGS += -I${.CURDIR}/../include -DHAVE_CONFIG_H
LDADD += -lntp
DPADD += ${LIBNTP}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
