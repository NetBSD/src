#	$NetBSD: Makefile,v 1.3 2001/05/14 18:39:14 jdolecek Exp $	

.PATH:	${CURDIR}/..

INCS=		readline.h
INCSDIR=	/usr/include/readline
SYMLINKS=	${INCSDIR}/readline.h	${INCSDIR}/history.h

MKOBJ=	no

incinstall:: linksinstall

.include <bsd.prog.mk>
