#	$NetBSD: Makefile.inc,v 1.3 1997/10/22 02:31:35 lukem Exp $

LIBCOMMON != cd ${.CURDIR}/../common_source;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CPPFLAGS+=-DV7 -I${.CURDIR}/../common_source
DPADD+=	${LIBTERM} ${LIBCOMPAT} ${LIBCOMMON}/libcommon.a
LDADD+=	-ltermcap -lcompat  -L${LIBCOMMON} -lcommon

HIDEGAME=hidegame

.include "../Makefile.inc"
