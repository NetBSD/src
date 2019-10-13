#	$NetBSD: Makefile,v 1.2 2019/10/13 07:28:17 mrg Exp $

.include <bsd.own.mk>

PROG=	utoppya
SRCS=	utoppya.c progressbar.c

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/ftp -DSTANDALONE_PROGRESS

.PATH:	${NETBSDSRCDIR}/usr.bin/ftp

COPTS.utoppya.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
