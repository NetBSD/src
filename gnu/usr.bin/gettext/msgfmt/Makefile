#	$NetBSD: Makefile,v 1.2 2001/12/13 00:30:28 thorpej Exp $

PROG=	msgfmt
SRCS=	msgfmt.c open-po.c po-gram.gen.c po-hash.gen.c po-lex.c po.c \
	str-list.c message.c dir-list.c

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=	-I${DIST}/gettext/intl
