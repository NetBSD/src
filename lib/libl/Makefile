#	$NetBSD: Makefile,v 1.9 2001/12/12 00:51:04 tv Exp $

LIB=		l
SRCS=		libmain.c libyywrap.c
NOPIC=		# defined

LINKS=		/usr/lib/libl.a /usr/lib/libfl.a

.PATH:		${.CURDIR}/../../usr.bin/lex
.include <bsd.lib.mk>

.if ${MKPROFILE} != "no"
LINKS+=		${LIBDIR}/libl_p.a ${LIBDIR}/libfl_p.a
.endif
