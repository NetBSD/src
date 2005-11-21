#	$NetBSD: Makefile.inc,v 1.1.2.2 2005/11/21 20:36:31 tron Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

CPPFLAGS+=-DNAMED_CONFFILE=\"${SYSCONFDIR}/named.conf\"
BINDIR=/usr/sbin
CHECK=${IDIST}/bin/check

.PATH: ${CHECK}

PROG=${BASE}
SRCS=${BASE}.c check-tool.c
MAN=${BASE}.8
