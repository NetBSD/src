#	$NetBSD: Makefile,v 1.2 2001/12/13 00:31:40 thorpej Exp $

PROG=	xgettext
SRCS=	message.c open-po.c po-gram.gen.c po-hash.gen.c po-lex.c \
	po.c str-list.c xget-lex.c xgettext.c dir-list.c

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=	-I${DIST}/gettext/intl
