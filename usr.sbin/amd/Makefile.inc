#	$NetBSD: Makefile.inc,v 1.17 2002/03/22 18:10:25 thorpej Exp $

WARNS?=	1
WFORMAT=2

.include <bsd.own.mk>

IDIST=	${.CURDIR}/../../../dist/am-utils

LIBAMUOBJDIR!=cd ${.CURDIR}/../libamu && ${PRINTOBJDIR}

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}/libamu \
	-I${IDIST}/include -I${LIBAMUOBJDIR}

.if (${USE_HESIOD} != "no")
CPPFLAGS+=-DHAVE_MAP_HESIOD=1
.endif

.if (${USE_YP} != "no")
CPPFLAGS+=-DHAVE_MAP_NIS=1
.endif

.if !defined(LIB) || empty(LIB)
LDADD+=	-L${LIBAMUOBJDIR} -lamu
.if ${MKPIC} == "no"
DPADD+=	${LIBAMUOBJDIR}/libamu.a
.else
DPADD+=	${LIBAMUOBJDIR}/libamu_pic.a
.endif
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
