#	$NetBSD: Makefile.inc,v 1.1.10.1 2001/05/15 21:04:11 he Exp $

LIBAUDIO != cd ${.CURDIR}/../common;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBAUDIO}/libaudio.a
LDADD+=	-L${LIBAUDIO} -laudio

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
