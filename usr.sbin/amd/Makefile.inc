#	$NetBSD: Makefile.inc,v 1.4 1997/09/22 22:09:51 christos Exp $
LIBAMU != cd ${.CURDIR}/../libamu;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CFLAGS += -I${.CURDIR}/../include -I${.CURDIR} -I${LIBAMU} -I. -DHAVE_CONFIG_H
LDADD += -L${LIBAMU} -lamu -lrpcsvc -ll
DPADD += ${LIBAMU}/libamu.a ${LIBRPCSVC} ${LIBL}
BINDIR = /usr/sbin
