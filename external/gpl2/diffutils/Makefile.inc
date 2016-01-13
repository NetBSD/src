#	$NetBSD: Makefile.inc,v 1.1 2016/01/13 03:39:28 christos Exp $

WARNS=	0

IDIST=	${NETBSDSRCDIR}/external/gpl2/diffutils/dist

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H \
		-I${IDIST}/../include -I${IDIST}/lib
