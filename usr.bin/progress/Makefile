# $NetBSD: Makefile,v 1.1 2003/01/22 00:14:12 jhawk Exp $
PROG=progress
SRCS=progress.c progressbar.c

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/ftp -DSTANDALONE_PROGRESS

.PATH: ${NETBSDSRCDIR}/usr.bin/ftp

.include <bsd.prog.mk>
