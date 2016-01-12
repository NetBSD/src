#	$NetBSD: Makefile,v 1.1 2016/01/12 22:58:01 christos Exp $

PROG=	msgfmt

.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=-I${DIST}/gettext-runtime/intl

CWARNFLAGS.clang+=	-Wno-format-extra-args
