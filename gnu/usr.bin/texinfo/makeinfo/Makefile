#	$NetBSD: Makefile,v 1.3 2001/07/25 16:47:50 assar Exp $

PROG=		makeinfo
SRCS=		cmds.c defun.c files.c footnote.c \
		html.c index.c insertion.c lang.c \
		macro.c makeinfo.c multi.c node.c \
		sectioning.c toc.c 

CPPFLAGS+=	-I${DIST}/texinfo/makeinfo
MKMAN=		no

.include <bsd.prog.mk>

.PATH: ${DIST}/texinfo/makeinfo
