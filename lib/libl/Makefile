#	$NetBSD: Makefile,v 1.11 2002/08/19 09:41:32 lukem Exp $

NOPIC=		# defined

.include <bsd.own.mk>

LIB=		l
SRCS=		libmain.c libyywrap.c

LINKS=		${LIBDIR}/libl.a ${LIBDIR}/libfl.a

.PATH:		${NETBSDSRCDIR}/usr.bin/lex

.include <bsd.lib.mk>

.if ${MKPROFILE} != "no"
LINKS+=		${LIBDIR}/libl_p.a ${LIBDIR}/libfl_p.a
.endif
