#	$NetBSD: Makefile,v 1.2 2001/12/13 00:31:39 thorpej Exp $

PROG=	msgunfmt
SRCS=	message.c msgunfmt.c str-list.c

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=	-I${DIST}/gettext/intl
