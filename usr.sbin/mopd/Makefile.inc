#	$NetBSD: Makefile.inc,v 1.4 1997/10/16 07:36:19 lukem Exp $

LIBCOMMON != cd ${.CURDIR}/../common;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CFLAGS+=-I${.CURDIR}/../common -I${.CURDIR}/..
DPADD+=	${LIBKVM} ${LIBCOMMON}/libcommon.a
LDADD+=	-lkvm -L${LIBCOMMON} -lcommon

BINDIR?=	/usr/sbin
