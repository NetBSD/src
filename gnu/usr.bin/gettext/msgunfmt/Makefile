#	$NetBSD: Makefile,v 1.1 2000/11/02 01:09:04 itojun Exp $

.include <bsd.own.mk>

PROG=	msgunfmt
SRCS=	message.c msgunfmt.c str-list.c

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=	-I${DIST}/gettext/intl
