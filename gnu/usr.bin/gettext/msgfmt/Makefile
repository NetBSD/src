#	$NetBSD: Makefile,v 1.4 2012/05/09 07:59:23 joerg Exp $

PROG=	msgfmt

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=-I${DIST}/gettext/gettext-runtime/intl

CWARNFLAGS.clang+=	-Wno-format-extra-args
