#	$NetBSD: Makefile,v 1.1 2016/01/12 22:58:02 christos Exp $

PROG=	msggrep

PROGDPLIBS+= grep ${.CURDIR}/../../lib/libgrep

.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=-I${DIST}/gettext-tools/libgrep
