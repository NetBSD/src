#	$NetBSD: Makefile.inc,v 1.3 1997/04/21 03:07:28 christos Exp $
CFLAGS += -I${.CURDIR}/../include -DHAVE_CONFIG_H
LDADD += -lntp
DPADD += ${LIBNTP}
BINDIR=/usr/sbin
