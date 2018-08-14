#	$NetBSD: Makefile.inc,v 1.2 2018/08/14 11:52:41 christos Exp $

LDADD+=-lns
DPADD+=${LIBNS}

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

CPPFLAGS+=-DNAMED_CONFFILE=\"${SYSCONFDIR}/named.conf\"
BINDIR=/usr/sbin
CHECK=${IDIST}/bin/check

.PATH: ${CHECK}

PROG=${BASE}
SRCS=${BASE}.c check-tool.c
MAN=${BASE}.8
