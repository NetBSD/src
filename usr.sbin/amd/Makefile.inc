#	$NetBSD: Makefile.inc,v 1.6 1997/10/18 06:42:28 lukem Exp $
LIBAMU != cd ${.CURDIR}/../libamu;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CFLAGS += -I${.CURDIR}/../include -I${.CURDIR} -I${LIBAMU} -I. -DHAVE_CONFIG_H
LDADD += -L${LIBAMU} -lamu -lrpcsvc -ll
DPADD += ${LIBAMU}/libamu.a ${LIBRPCSVC} ${LIBL}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
