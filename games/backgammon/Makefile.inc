#	$NetBSD: Makefile.inc,v 1.8 2012/10/13 19:25:22 dholland Exp $

.include <bsd.own.mk>

LIBCOMMON != cd ${.CURDIR}/../common_source; ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common_source
DPADD+= ${LIBCOMMON}/libcommon.a ${LIBTERMINFO}
LDADD+=	-L${LIBCOMMON} -lcommon -lterminfo

HIDEGAME=hidegame

.include "../Makefile.inc"
