#	$NetBSD: Makefile,v 1.7 2002/04/23 08:19:15 msaitoh Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	rtquery
MAN=	rtquery.8
BINDIR?=	/usr/sbin
.if !defined(NOPIC)
LDSTATIC?=
.endif
#COPTS=	-g -DDEBUG -Wall

.include "../../Makefile.inc"
.include <bsd.prog.mk>
