#	$NetBSD: Makefile,v 1.1 2000/11/02 01:09:04 itojun Exp $

.include <bsd.own.mk>

PROG=	msgfmt
SRCS=	msgfmt.c open-po.c po-gram.gen.c po-hash.gen.c po-lex.c po.c \
	str-list.c message.c dir-list.c

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=	-I${DIST}/gettext/intl
