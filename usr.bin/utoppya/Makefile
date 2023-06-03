#	$NetBSD: Makefile,v 1.3 2023/06/03 09:09:19 lukem Exp $

.include <bsd.own.mk>

PROG=	utoppya
SRCS=	utoppya.c progressbar.c

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/ftp -DSTANDALONE_PROGRESS

.PATH:	${NETBSDSRCDIR}/usr.bin/ftp

COPTS.utoppya.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
