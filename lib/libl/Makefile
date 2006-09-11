#	$NetBSD: Makefile,v 1.13 2006/09/11 22:24:09 dbj Exp $

NOPIC=		# defined

.include <bsd.own.mk>

LIB=		l
SRCS=		libmain.c libyywrap.c

.if ${MKSTATICLIB} != "no"
LINKS=		${LIBDIR}/libl.a ${LIBDIR}/libfl.a
.endif

.PATH:		${NETBSDSRCDIR}/usr.bin/lex

.if ${MKPROFILE} != "no"
LINKS+=		${LIBDIR}/libl_p.a ${LIBDIR}/libfl_p.a
.endif

.include <bsd.lib.mk>
