#	$NetBSD: Makefile.inc,v 1.8 1997/10/25 06:58:22 lukem Exp $

LIBCOMMON != cd ${.CURDIR}/../common;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBKVM} ${LIBCOMMON}/libcommon.a
LDADD+=	-lkvm -L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
