#	$NetBSD: Makefile.inc,v 1.5 1997/09/26 16:59:42 christos Exp $
WARNS ?= 1
LIBAMU != cd ${.CURDIR}/../libamu;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CFLAGS += -I${.CURDIR}/../include -I${.CURDIR} -I${LIBAMU} -I. -DHAVE_CONFIG_H
LDADD += -L${LIBAMU} -lamu -lrpcsvc -ll
DPADD += ${LIBAMU}/libamu.a ${LIBRPCSVC} ${LIBL}
BINDIR = /usr/sbin
