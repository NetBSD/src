#	$NetBSD: Makefile,v 1.1 2002/08/27 14:12:15 takemura Exp $

.if ${MACHINE} == "hpcarm" || ${MACHINE} == "hpcmips" || ${MACHINE} == "hpcsh"
PROG=	tpctl
SRCS=	main.c fb.c tp.c data.c
.endif

MAN=	tpctl.8

.include <bsd.prog.mk>
