#	$NetBSD: Makefile.inc,v 1.5 1997/10/16 23:24:18 lukem Exp $

LIBCOMMON != cd ${.CURDIR}/../common;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBKVM} ${LIBCOMMON}/libcommon.a
LDADD+=	-lkvm -L${LIBCOMMON} -lcommon

BINDIR?=	/usr/sbin
WARNS?=		1
