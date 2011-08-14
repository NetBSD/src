#	$NetBSD: Makefile,v 1.18 2011/08/14 10:53:17 christos Exp $
#	@(#)Makefile	8.3 (Berkeley) 5/8/95

.include <bsd.own.mk>

PROG=	df
.PATH:  ${NETBSDSRCDIR}/bin/csh
SRCS=	df.c strpct.c
COPTS.strpct.c = -Wno-format-nonliteral

.include <bsd.prog.mk>
