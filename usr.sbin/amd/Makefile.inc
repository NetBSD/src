#	$NetBSD: Makefile.inc,v 1.1.1.7 2000/11/20 00:16:16 wiz Exp $

WARNS?=	1

IDIST=	${.CURDIR}/../../../dist/am-utils

LIBAMUOBJDIR!=cd ${.CURDIR}/../libamu; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}/libamu \
	-I${IDIST}/include -I${LIBAMUOBJDIR}

.include <bsd.own.mk>

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
