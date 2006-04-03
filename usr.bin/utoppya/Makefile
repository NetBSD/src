#	$NetBSD: Makefile,v 1.1 2006/04/03 08:15:48 scw Exp $

.include <bsd.own.mk>

PROG=	utoppya
SRCS=	utoppya.c progressbar.c

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/ftp -DSTANDALONE_PROGRESS

.PATH:	${NETBSDSRCDIR}/usr.bin/ftp

.include <bsd.prog.mk>
