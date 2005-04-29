#	$NetBSD: Makefile.inc,v 1.6 2005/04/29 15:05:01 christos Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR} -I${.CURDIR}/../include \
		-I${DIST}/gettext/gettext-tools/lib -DLOCALEDIR=\"/usr/share/locale\"

BINDIR?=	/usr/bin
