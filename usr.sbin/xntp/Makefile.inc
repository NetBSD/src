#	$NetBSD: Makefile.inc,v 1.4 1997/06/23 17:34:18 christos Exp $
CFLAGS += -I${.CURDIR}/../include -DHAVE_CONFIG_H
LDADD += -lntp
DPADD += ${LIBNTP}
BINDIR = /usr/sbin
