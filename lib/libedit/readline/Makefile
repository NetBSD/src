#	$NetBSD: Makefile,v 1.6 2002/08/19 13:35:07 lukem Exp $	

NOOBJ=		# defined

.include <bsd.own.mk>

.PATH:	${NETBSDSRCDIR}/lib/libedit

INCS=		readline.h
INCSDIR=	/usr/include/readline
SYMLINKS=	readline.h ${INCSDIR}/history.h


incinstall:: linksinstall

.include <bsd.prog.mk>
