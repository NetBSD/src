#	$NetBSD: Makefile,v 1.10 2002/08/19 13:54:37 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.obj.mk>

PROG=	tftpd
SRCS=	tftpd.c tftpsubs.c
MAN=	tftpd.8
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/tftp
.PATH:	${NETBSDSRCDIR}/usr.bin/tftp

.include <bsd.prog.mk>
