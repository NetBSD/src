#	$NetBSD: Makefile.inc,v 1.5 1998/02/02 14:14:55 christos Exp $

LIBCOMMON != cd ${.CURDIR}/../common_source;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CPPFLAGS+=-DV7 -I${.CURDIR}/../common_source
DPADD+= ${LIBCOMMON}/libcommon.a ${LIBTERMCAP}
LDADD+=	-L${LIBCOMMON} -lcommon -ltermcap 

HIDEGAME=hidegame

.include "../Makefile.inc"
