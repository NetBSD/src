#	$NetBSD: Makefile,v 1.1.86.1 2020/04/13 08:05:49 martin Exp $

.include <bsd.own.mk>

PROG=	utoppya
SRCS=	utoppya.c progressbar.c

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/ftp -DSTANDALONE_PROGRESS

.PATH:	${NETBSDSRCDIR}/usr.bin/ftp

COPTS.utoppya.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
