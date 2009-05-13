#	$NetBSD: Makefile.inc,v 1.1.2.2 2009/05/13 18:49:09 jym Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

CPPFLAGS+=-DNAMED_CONFFILE=\"${SYSCONFDIR}/named.conf\"
BINDIR=/usr/sbin
CHECK=${IDIST}/bin/check

.PATH: ${CHECK}

PROG=${BASE}
SRCS=${BASE}.c check-tool.c
MAN=${BASE}.8
