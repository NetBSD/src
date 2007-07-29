#	$NetBSD: Makefile,v 1.4 2007/07/29 14:37:25 nonaka Exp $

.if ${MACHINE} == "hpcarm" || ${MACHINE} == "hpcmips" || ${MACHINE} == "hpcsh" \
	|| ${MACHINE} == "zaurus"
PROG=	tpctl
SRCS=	main.c fb.c tp.c data.c
.endif

MAN=	tpctl.8

.include <bsd.prog.mk>
