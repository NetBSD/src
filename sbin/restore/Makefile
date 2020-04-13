#	$NetBSD: Makefile,v 1.26.42.1 2020/04/13 08:03:22 martin Exp $
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

COPTS.interactive.c+=	${GCC_NO_STRINGOP_TRUNCATION}
COPTS.dirs.c+=		${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
