#	$NetBSD: Makefile.inc,v 1.1.1.1 1997/07/24 21:20:04 christos Exp $
LIBAMU != cd ${.CURDIR}/../libamu;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CFLAGS += -I${.CURDIR}/../include -I${.CURDIR} -I${LIBAMU} -I. -DHAVE_CONFIG_H
LDADD += -L${LIBAMU} -lamu -lrpcsvc
DPADD += ${LIBAMU}/libamu.a
BINDIR = /usr/sbin
