#	$NetBSD: Makefile.inc,v 1.1 2006/02/04 16:22:53 christos Exp $

.include <bsd.own.mk>

WARNS?=	0

IDIST=	${NETBSDSRCDIR}/gnu/dist/xcvs

CPPFLAGS+= -DSETXID_SUPPORT -DHAVE_CONFIG_H \
	-I${.CURDIR}/../include -I${IDIST}/lib \
	-I${IDIST}/diff -I${IDIST}/src

DOBJDIR!=cd $(.CURDIR)/../libdiff && ${PRINTOBJDIR}
COBJDIR!=cd $(.CURDIR)/../libcvs && ${PRINTOBJDIR}

LIBDIFF=	${DOBJDIR}/libdiff.a
LIBCVS=		${COBJDIR}/libcvs.a

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
