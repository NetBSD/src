#	$NetBSD: Makefile.inc,v 1.1 1997/10/10 08:59:33 lukem Exp $

LIBCOMMON != cd ${.CURDIR}/../common_source;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CFLAGS+=-DV7 -I${.CURDIR}/../common_source
DPADD+=	${LIBTERM} ${LIBCOMPAT} ${LIBCOMMON}/libcommon_source.a
LDADD+=	-ltermcap -lcompat  -L${LIBCOMMON} -lcommon_source

HIDEGAME=hidegame

.include "../Makefile.inc"
