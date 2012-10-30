#	$NetBSD: Makefile.inc,v 1.7.6.1 2012/10/30 18:58:17 yamt Exp $

.include <bsd.own.mk>

LIBCOMMON != cd ${.CURDIR}/../common_source; ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common_source
DPADD+= ${LIBCOMMON}/libcommon.a ${LIBTERMINFO}
LDADD+=	-L${LIBCOMMON} -lcommon -lterminfo

HIDEGAME=hidegame

.include "../Makefile.inc"
