#	$NetBSD: Makefile,v 1.20 2001/02/04 21:38:01 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	restore
LINKS=	${BINDIR}/restore ${BINDIR}/rrestore
CPPFLAGS+=-DRRESTORE -DRDUMP
SRCS=	main.c interactive.c restore.c dirs.c symtab.c tape.c utilities.c \
	dumprmt.c
MAN=	restore.8
MLINKS+=restore.8 rrestore.8
.PATH:	${.CURDIR}/../dump

.include <bsd.prog.mk>
