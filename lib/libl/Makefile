#	$NetBSD: Makefile,v 1.12 2005/05/07 00:23:01 lukem Exp $

NOPIC=		# defined

.include <bsd.own.mk>

LIB=		l
SRCS=		libmain.c libyywrap.c

.if ${MKSTATICLIB} != "no"
LINKS=		${LIBDIR}/libl.a ${LIBDIR}/libfl.a
.endif

.PATH:		${NETBSDSRCDIR}/usr.bin/lex

.include <bsd.lib.mk>

.if ${MKPROFILE} != "no"
LINKS+=		${LIBDIR}/libl_p.a ${LIBDIR}/libfl_p.a
.endif
