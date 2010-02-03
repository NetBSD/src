#	$NetBSD: Makefile.inc,v 1.7 2010/02/03 15:34:38 roy Exp $

.include <bsd.own.mk>

LIBCOMMON != cd ${.CURDIR}/../common_source; ${PRINTOBJDIR}
CPPFLAGS+=-DV7 -I${.CURDIR}/../common_source
DPADD+= ${LIBCOMMON}/libcommon.a ${LIBTERMINFO}
LDADD+=	-L${LIBCOMMON} -lcommon -lterminfo

HIDEGAME=hidegame

.include "../Makefile.inc"
