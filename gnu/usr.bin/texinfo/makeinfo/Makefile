#	$NetBSD: Makefile,v 1.5 2003/01/17 15:32:24 wiz Exp $

PROG=		makeinfo
SRCS=		cmds.c defun.c files.c footnote.c \
		html.c index.c insertion.c lang.c \
		macro.c makeinfo.c multi.c node.c \
		sectioning.c toc.c xml.c

CPPFLAGS+=	-I${IDIST}/makeinfo

.include <bsd.prog.mk>

.PATH: ${IDIST}/makeinfo ${IDIST}/doc
