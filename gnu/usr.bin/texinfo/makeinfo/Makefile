#	$NetBSD: Makefile,v 1.4 2001/09/02 22:16:58 wiz Exp $

PROG=		makeinfo
SRCS=		cmds.c defun.c files.c footnote.c \
		html.c index.c insertion.c lang.c \
		macro.c makeinfo.c multi.c node.c \
		sectioning.c toc.c 

CPPFLAGS+=	-I${DIST}/texinfo/makeinfo

.include <bsd.prog.mk>

.PATH: ${DIST}/texinfo/makeinfo ${DIST}/texinfo/doc
