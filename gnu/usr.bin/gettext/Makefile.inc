#	$NetBSD: Makefile.inc,v 1.4 2001/12/12 01:49:04 tv Exp $

DIST=		${.CURDIR}/../../../dist

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR} -I${.CURDIR}/../include \
		-I${DIST}/gettext/lib -DLOCALEDIR=\"/usr/share/locale\"

NOMAN=		# defined

BINDIR?=	/usr/bin
