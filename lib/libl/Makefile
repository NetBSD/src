#	$NetBSD: Makefile,v 1.6 1999/02/12 11:22:55 lukem Exp $

LIB=	l
SRCS=	libmain.c libyywrap.c
NOPIC=

LINKS=	/usr/lib/libl.a /usr/lib/libfl.a

.PATH:	${.CURDIR}/../../usr.bin/lex
.include <bsd.lib.mk>

.if ${MKPROFILE} != "no"
LINKS+=	/usr/lib/libl_p.a /usr/lib/libfl_p.a
.endif
