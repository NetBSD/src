#	$NetBSD: Makefile,v 1.8 1999/07/12 20:17:09 itojun Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	tftpd
SRCS=	tftpd.c tftpsubs.c
MAN=	tftpd.8
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/tftp
.PATH:	${.CURDIR}/../../usr.bin/tftp

# for portability across old sockaddr_storage definition
CPPFLAGS+=-Dss_len=__ss_len -Dss_family=__ss_family

.include <bsd.prog.mk>
