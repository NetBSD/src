#	$NetBSD: Makefile.inc,v 1.7 1997/10/18 06:42:52 lukem Exp $

LIBCOMMON != cd ${.CURDIR}/../common;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBKVM} ${LIBCOMMON}/libcommon.a
LDADD+=	-lkvm -L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
