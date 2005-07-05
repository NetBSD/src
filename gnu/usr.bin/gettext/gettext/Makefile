#	$NetBSD: Makefile,v 1.4 2005/07/05 03:10:11 christos Exp $

PROG=	gettext

CLEANFILES+=gettext.1

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

gettext.1: ${DIST}/gettext/gettext-runtime/man/gettext.1.in
	cp $> $@
