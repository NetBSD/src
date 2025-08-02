#	$NetBSD: Makefile.inc,v 1.8.42.1 2025/08/02 05:54:25 perseant Exp $

.include <bsd.own.mk>

LIBCOMMON != cd ${.CURDIR}/../common_source; ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common_source
DPADD+= ${LIBCOMMON}/libcommon.a ${LIBTERMINFO}
LDADD+=	-L${LIBCOMMON} -lcommon -lterminfo

HIDEGAME=hidegame

LINTFLAGS+=	-w		# turn warnings into errors

.include "../Makefile.inc"
