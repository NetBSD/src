#	$NetBSD: Makefile.inc,v 1.4 2003/01/26 23:53:44 wiz Exp $

.include <bsd.own.mk>

WARNS=	0

IDIST=	${NETBSDSRCDIR}/gnu/dist/grep

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H \
		-I${.CURDIR}/../include -I${IDIST}/lib

DOBJDIR!=	cd $(.CURDIR)/../lib && ${PRINTOBJDIR}

LIBGREPUTILS=	${DOBJDIR}/libgreputils.a

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
