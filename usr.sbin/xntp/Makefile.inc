#	$NetBSD: Makefile.inc,v 1.2 1997/04/18 14:06:15 christos Exp $
CFLAGS += -I${.CURDIR}/../include -DHAVE_CONFIG_H
LDADD += -lntp -lkvm
DPADD += ${LIBNTP} ${LIBKVM}
BINDIR=/usr/sbin
