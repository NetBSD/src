#	$NetBSD: Makefile.inc,v 1.1.1.1 2003/01/26 00:40:40 wiz Exp $

.include <bsd.own.mk>

WARNS=	0

IDIST=	${NETBSDSRCDIR}/gnu/dist/diffutils-2.8.1

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H \
		-I${.CURDIR}/../include -I${IDIST}/lib

DOBJDIR!=	cd $(.CURDIR)/../lib && ${PRINTOBJDIR}

LIBDIFFUTILS=	${DOBJDIR}/libdiffutils.a

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
