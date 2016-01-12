#	$NetBSD: Makefile,v 1.1 2016/01/12 22:58:01 christos Exp $

PROG=	gettext

CLEANFILES+=gettext.1

.include "${.CURDIR}/../Makefile.inc.prog"

gettext.1: ${DIST}/gettext/gettext-runtime/man/gettext.1.in
	cp $> $@
