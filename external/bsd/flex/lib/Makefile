#	$NetBSD: Makefile,v 1.1 2009/10/26 00:49:04 christos Exp $

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

.if ${MKDEBUG} != "no"
LINKS+=		${LIBDIR}/libl_g.a ${LIBDIR}/libfl_g.a
.endif

.include <bsd.lib.mk>
