#	$NetBSD: Makefile.inc,v 1.1.1.3 1998/08/12 13:35:18 christos Exp $
WARNS?=	1
CPPFLAGS += -I${.CURDIR}/../include -DHAVE_CONFIG_H
LDADD += -lntp
DPADD += ${LIBNTP}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
