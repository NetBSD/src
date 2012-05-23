#	$NetBSD: Makefile,v 1.3.42.1 2012/05/23 10:07:29 yamt Exp $

PROG=	msgfmt

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=-I${DIST}/gettext/gettext-runtime/intl

CWARNFLAGS.clang+=	-Wno-format-extra-args
