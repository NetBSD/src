#	$NetBSD: Makefile.inc,v 1.2.2.2 1997/11/08 05:47:11 lukem Exp $

LIBCOMMON != cd ${.CURDIR}/../common_source;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CPPFLAGS+=-DV7 -I${.CURDIR}/../common_source
DPADD+=	${LIBTERM} ${LIBCOMPAT} ${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon -ltermcap -lcompat

HIDEGAME=hidegame

.include "../Makefile.inc"
