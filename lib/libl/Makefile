#	$NetBSD: Makefile,v 1.10 2001/12/27 11:39:20 lukem Exp $

LIB=		l
SRCS=		libmain.c libyywrap.c
NOPIC=		# defined

LINKS=		${LIBDIR}/libl.a ${LIBDIR}/libfl.a

.PATH:		${.CURDIR}/../../usr.bin/lex
.include <bsd.lib.mk>

.if ${MKPROFILE} != "no"
LINKS+=		${LIBDIR}/libl_p.a ${LIBDIR}/libfl_p.a
.endif
