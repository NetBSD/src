#	$NetBSD: Makefile,v 1.12 2009/03/16 02:24:57 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

WARNS?=	2		# XXX: setjmp clobber warnings

.include <bsd.own.mk>

PROG=	tftpd
SRCS=	tftpd.c tftpsubs.c
MAN=	tftpd.8
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/tftp
.PATH:	${NETBSDSRCDIR}/usr.bin/tftp

.include <bsd.prog.mk>
