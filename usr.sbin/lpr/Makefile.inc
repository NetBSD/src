#	$NetBSD: Makefile.inc,v 1.3 1997/10/18 06:42:51 lukem Exp $

LIBCOMMON != cd ${.CURDIR}/../common_source;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CFLAGS+=-I${.CURDIR}/../common_source
DPADD+=	${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
