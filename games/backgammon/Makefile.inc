#	$NetBSD: Makefile.inc,v 1.9 2024/08/22 20:46:40 rillig Exp $

.include <bsd.own.mk>

LIBCOMMON != cd ${.CURDIR}/../common_source; ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common_source
DPADD+= ${LIBCOMMON}/libcommon.a ${LIBTERMINFO}
LDADD+=	-L${LIBCOMMON} -lcommon -lterminfo

HIDEGAME=hidegame

LINTFLAGS+=	-w		# turn warnings into errors

.include "../Makefile.inc"
