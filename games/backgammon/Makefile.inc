#	$NetBSD: Makefile.inc,v 1.6 2000/12/30 17:51:26 sommerfeld Exp $

.include <bsd.own.mk>

LIBCOMMON != cd ${.CURDIR}/../common_source; ${PRINTOBJDIR}
CPPFLAGS+=-DV7 -I${.CURDIR}/../common_source
DPADD+= ${LIBCOMMON}/libcommon.a ${LIBTERMCAP}
LDADD+=	-L${LIBCOMMON} -lcommon -ltermcap 

HIDEGAME=hidegame

.include "../Makefile.inc"
