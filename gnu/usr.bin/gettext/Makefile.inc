#	$NetBSD: Makefile.inc,v 1.3 2000/11/03 07:07:58 dan Exp $

DIST=		${.CURDIR}/../../../dist

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR} -I${.CURDIR}/../include \
		-I${DIST}/gettext/lib -DLOCALEDIR=\"/usr/share/locale\"

MKMAN=		no

BINDIR?=	/usr/bin
