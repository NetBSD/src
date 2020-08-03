#	$NetBSD: Makefile.inc,v 1.3 2020/08/03 17:23:35 christos Exp $

LDADD+=-lns
DPADD+=${LIBNS}

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

CPPFLAGS+=-DNAMED_CONFFILE=\"${SYSCONFDIR}/named.conf\"
BINDIR=/usr/sbin
CHECK=${IDIST}/bin/check

.PATH: ${CHECK} ${IDIST}/doc/man

PROG=${BASE}
SRCS=${BASE}.c check-tool.c
MAN=${BASE}.8
