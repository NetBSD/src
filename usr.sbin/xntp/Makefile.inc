#	$NetBSD: Makefile.inc,v 1.1.1.1 1997/04/18 13:29:03 christos Exp $
CFLAGS += -I${.CURDIR}/../include -DHAVE_CONFIG_H
LDADD += -lntp -lkvm
DPADD += ${LIBNTP} ${LIBKVM}
