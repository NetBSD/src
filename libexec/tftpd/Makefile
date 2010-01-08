#	$NetBSD: Makefile,v 1.13 2010/01/08 21:05:14 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

WARNS?=	5

.include <bsd.own.mk>

PROG=	tftpd
SRCS=	tftpd.c tftpsubs.c
MAN=	tftpd.8
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/tftp
.PATH:	${NETBSDSRCDIR}/usr.bin/tftp

.include <bsd.prog.mk>
