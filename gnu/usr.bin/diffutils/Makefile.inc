#	$NetBSD: Makefile.inc,v 1.2 2003/01/26 06:51:04 elric Exp $

.include <bsd.own.mk>

WARNS=	0

IDIST=	${NETBSDSRCDIR}/gnu/dist/diffutils

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H \
		-I${.CURDIR}/../include -I${IDIST}/lib

DOBJDIR!=	cd $(.CURDIR)/../lib && ${PRINTOBJDIR}

LIBDIFFUTILS=	${DOBJDIR}/libdiffutils.a

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
