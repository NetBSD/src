#	$NetBSD: Makefile,v 1.23.24.1 2008/09/18 04:28:29 wrstuden Exp $
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

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.tape.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
