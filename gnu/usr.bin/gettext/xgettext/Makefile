#	$NetBSD: Makefile,v 1.1 2000/11/02 01:09:04 itojun Exp $

.include <bsd.own.mk>

PROG=	xgettext
SRCS=	message.c open-po.c po-gram.gen.c po-hash.gen.c po-lex.c \
	po.c str-list.c xget-lex.c xgettext.c dir-list.c

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=	-I${DIST}/gettext/intl
