#	$NetBSD: Makefile.inc,v 1.8 1998/03/26 07:07:55 thorpej Exp $

WARNS?=	0
CPPFLAGS += -I${.CURDIR}/../include -DHAVE_CONFIG_H
LDADD += -lntp
DPADD += ${LIBNTP}

# Some later versions (at least 2.7.2.3.f.1) of GCC have a `restrict'
# keyword?!
CFLAGS+=-ansi

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
