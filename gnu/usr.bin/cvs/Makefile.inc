#	$NetBSD: Makefile.inc,v 1.3 2002/03/06 02:40:06 wiz Exp $

.include <bsd.own.mk>

WARNS?=	0

IDIST=	${.CURDIR}/../../../dist/cvs

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}/lib \
	-I${IDIST}/diff -I${IDIST}/src

DOBJDIR!=cd $(.CURDIR)/../libdiff && ${PRINTOBJDIR}
COBJDIR!=cd $(.CURDIR)/../libcvs && ${PRINTOBJDIR}

LIBDIFF=	${DOBJDIR}/libdiff.a
LIBCVS=		${COBJDIR}/libcvs.a

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
