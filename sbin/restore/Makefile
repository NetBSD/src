#	$NetBSD: Makefile,v 1.22 2002/08/19 10:17:00 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	restore
LINKS=	${BINDIR}/restore ${BINDIR}/rrestore
CPPFLAGS+=-DRRESTORE
SRCS=	main.c interactive.c restore.c dirs.c symtab.c tape.c utilities.c \
	dumprmt.c
MAN=	restore.8
MLINKS+=restore.8 rrestore.8
.PATH:	${NETBSDSRCDIR}/sbin/dump

.include <bsd.prog.mk>
