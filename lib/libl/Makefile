#	$NetBSD: Makefile,v 1.7 1999/02/13 02:54:30 lukem Exp $

LIB=	l
SRCS=	libmain.c libyywrap.c
MKPIC=	no

LINKS=	/usr/lib/libl.a /usr/lib/libfl.a

.PATH:	${.CURDIR}/../../usr.bin/lex
.include <bsd.lib.mk>

.if ${MKPROFILE} != "no"
LINKS+=	/usr/lib/libl_p.a /usr/lib/libfl_p.a
.endif
