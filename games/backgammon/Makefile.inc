#	$NetBSD: Makefile.inc,v 1.2.2.1 1997/11/06 03:32:21 mellon Exp $

LIBCOMMON != cd ${.CURDIR}/../common_source;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CFLAGS+=-DV7 -I${.CURDIR}/../common_source
DPADD+=	${LIBTERM} ${LIBCOMPAT} ${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon -ltermcap -lcompat

HIDEGAME=hidegame

.include "../Makefile.inc"
