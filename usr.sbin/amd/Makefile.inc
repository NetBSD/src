#	$NetBSD: Makefile.inc,v 1.1.1.6 1999/02/01 18:45:39 christos Exp $

.include <bsd.own.mk>

WARNS ?= 1
LIBAMU != cd ${.CURDIR}/../libamu;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CPPFLAGS += -I${.CURDIR}/../include -I${.CURDIR} -I${LIBAMU} -I. -DHAVE_CONFIG_H
CPPFLAGS += -DDEBUG
.if !defined(LIB) || empty(LIB)
LDADD += -L${LIBAMU} -lamu -lrpcsvc -ll
.if defined(NOPIC)
DPADD += ${LIBAMU}/libamu.a
.else
DPADD += ${LIBAMU}/libamu_pic.a
.endif
DPADD += ${LIBRPCSVC} ${LIBL}
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
