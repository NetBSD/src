#	$NetBSD: Makefile.inc,v 1.16 2001/01/07 08:00:57 enami Exp $

WARNS?=	1
WFORMAT=2

.include <bsd.own.mk>

IDIST=	${.CURDIR}/../../../dist/am-utils

LIBAMUOBJDIR!=cd ${.CURDIR}/../libamu && ${PRINTOBJDIR}

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}/libamu \
	-I${IDIST}/include -I${LIBAMUOBJDIR}

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
