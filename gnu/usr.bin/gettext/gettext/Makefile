#	$NetBSD: Makefile,v 1.2 2005/04/29 15:05:01 christos Exp $

PROG=	gettext

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

gettext.1: ${DIST}/gettext/gettext-runtime/man/gettext.1.in
	${INSTALL_FILE} $> $@

CLEANFILES+=gettext.1
