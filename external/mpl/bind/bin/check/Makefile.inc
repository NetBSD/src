#	$NetBSD: Makefile.inc,v 1.3.6.1 2024/02/25 15:42:51 martin Exp $

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
MAN?=${BASE}.1
