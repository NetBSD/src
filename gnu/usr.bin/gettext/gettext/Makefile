#	$NetBSD: Makefile,v 1.3 2005/05/02 11:00:55 hira Exp $

PROG=	gettext

CLEANFILES+=gettext.1

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

gettext.1: ${DIST}/gettext/gettext-runtime/man/gettext.1.in
	${INSTALL_FILE} $> $@
