#	$NetBSD: Makefile.inc,v 1.11 2001/01/09 08:52:57 enami Exp $

.include <bsd.own.mk>

LIBRCSDIR!=cd $(.CURDIR)/../lib && ${PRINTOBJDIR}
LIBRCS=${LIBRCSDIR}/librcs.a
CPPFLAGS+=-I${.CURDIR}/../lib
LDADD=	${LIBRCS}
DPADD=	${LIBRCS}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
