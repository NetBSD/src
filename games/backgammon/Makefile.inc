#	$NetBSD: Makefile.inc,v 1.2 1997/10/11 20:24:05 mycroft Exp $

LIBCOMMON != cd ${.CURDIR}/../common_source;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CFLAGS+=-DV7 -I${.CURDIR}/../common_source
DPADD+=	${LIBTERM} ${LIBCOMPAT} ${LIBCOMMON}/libcommon.a
LDADD+=	-ltermcap -lcompat  -L${LIBCOMMON} -lcommon

HIDEGAME=hidegame

.include "../Makefile.inc"
