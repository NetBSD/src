#	$NetBSD: Makefile.inc,v 1.2 1997/10/02 14:16:14 lukem Exp $

LIBCOMMON != cd ${.CURDIR}/../common;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon

BINDIR?=/usr/sbin
WARNS?=	1
