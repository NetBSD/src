#	$NetBSD: Makefile.inc,v 1.9 1998/04/12 20:45:05 tv Exp $

.include <bsd.own.mk>

WARNS ?= 1
LIBAMU != cd ${.CURDIR}/../libamu;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CPPFLAGS += -I${.CURDIR}/../include -I${.CURDIR} -I${LIBAMU} -I. -DHAVE_CONFIG_H
LDADD += -L${LIBAMU} -lamu -lrpcsvc -ll
.if defined(NOPIC)
DPADD += ${LIBAMU}/libamu.a
.else
DPADD += ${LIBAMU}/libamu_pic.a
.endif
DPADD += ${LIBRPCSVC} ${LIBL}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
