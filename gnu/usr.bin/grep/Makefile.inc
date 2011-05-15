#	$NetBSD: Makefile.inc,v 1.5 2011/05/15 14:57:56 christos Exp $

.include <bsd.own.mk>

WARNS=	0

IDIST=	${NETBSDSRCDIR}/gnu/dist/grep

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H \
		-I${.CURDIR}/../include -I${IDIST}/lib -I${IDIST}/src

DOBJDIR!=	cd $(.CURDIR)/../lib && ${PRINTOBJDIR}

LIBGREPUTILS=	${DOBJDIR}/libgreputils.a

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
