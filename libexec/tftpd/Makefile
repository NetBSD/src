#	$NetBSD: Makefile,v 1.5 1997/10/07 09:48:03 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	tftpd
SRCS=	tftpd.c tftpsubs.c
MAN=	tftpd.8
CFLAGS+=-I${.CURDIR}/../../usr.bin/tftp
WARNS?=	1
.PATH:	${.CURDIR}/../../usr.bin/tftp

.include <bsd.prog.mk>
