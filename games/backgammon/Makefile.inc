#	$NetBSD: Makefile.inc,v 1.7.12.1 2012/11/20 02:58:45 tls Exp $

.include <bsd.own.mk>

LIBCOMMON != cd ${.CURDIR}/../common_source; ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common_source
DPADD+= ${LIBCOMMON}/libcommon.a ${LIBTERMINFO}
LDADD+=	-L${LIBCOMMON} -lcommon -lterminfo

HIDEGAME=hidegame

.include "../Makefile.inc"
