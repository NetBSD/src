#	$NetBSD: Makefile.inc,v 1.1 2016/01/10 22:16:40 christos Exp $

.include <bsd.own.mk>

WARNS=	0

IDIST=	${NETBSDSRCDIR}/external/gpl2/grep/dist

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H \
		-I${.CURDIR}/../include -I${IDIST}/lib -I${IDIST}/src

DOBJDIR!=	cd $(.CURDIR)/../lib && ${PRINTOBJDIR}

LIBGREPUTILS=	${DOBJDIR}/libgreputils.a
BINDIR=	/usr/bin

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
