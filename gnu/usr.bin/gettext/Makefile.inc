#	$NetBSD: Makefile.inc,v 1.2 2000/11/02 20:17:01 veego Exp $

DIST=		${.CURDIR}/../../../dist

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR} -I${.CURDIR}/../include \
		-I${DIST}/gettext/lib -DLOCALEDIR=\"/usr/share/locale\"

MKMAN=		no

BINDIR=		${DESTDIR}/usr/bin
