#	$NetBSD: Makefile,v 1.6.2.1 1997/11/08 07:26:02 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	tftpd
SRCS=	tftpd.c tftpsubs.c
MAN=	tftpd.8
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/tftp
.PATH:	${.CURDIR}/../../usr.bin/tftp

.include <bsd.prog.mk>
