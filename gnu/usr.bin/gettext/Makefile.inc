#	$NetBSD: Makefile.inc,v 1.1 2000/11/02 01:09:01 itojun Exp $

DIST=		${.CURDIR}/../../../dist

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR} -I${.CURDIR}/../include \
		-I${DIST}/gettext/lib -DLOCALEDIR=\"/usr/share/locale\"

MKMAN=		no

BINDIR=		${DISTDIR}/usr/bin
