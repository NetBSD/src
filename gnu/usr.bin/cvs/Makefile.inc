#	$NetBSD: Makefile.inc,v 1.4 2002/09/19 03:09:34 lukem Exp $

.include <bsd.own.mk>

WARNS?=	0

IDIST=	${NETBSDSRCDIR}/gnu/dist/cvs

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}/lib \
	-I${IDIST}/diff -I${IDIST}/src

DOBJDIR!=cd $(.CURDIR)/../libdiff && ${PRINTOBJDIR}
COBJDIR!=cd $(.CURDIR)/../libcvs && ${PRINTOBJDIR}

LIBDIFF=	${DOBJDIR}/libdiff.a
LIBCVS=		${COBJDIR}/libcvs.a

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
