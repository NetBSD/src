#	$NetBSD: Makefile,v 1.19 2007/05/28 12:06:30 tls Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

USE_FORT?=yes	# network client

WARNS=	3
PROG=	rsh
SRCS=	rsh.c getport.c

.PATH:	${NETBSDSRCDIR}/usr.bin/rlogin

.include <bsd.prog.mk>
