#	$NetBSD: Makefile,v 1.8 1999/04/03 16:28:49 simonb Exp $

LIB=	l
SRCS=	libmain.c libyywrap.c
MKPIC=	no

LINKS=	/usr/lib/libl.a /usr/lib/libfl.a

.PATH:	${.CURDIR}/../../usr.bin/lex
.include <bsd.lib.mk>

.if ${MKPROFILE} != "no"
LINKS+=	${LIBDIR}/libl_p.a ${LIBDIR}/libfl_p.a
.endif
