#	$NetBSD: Makefile,v 1.6 2004/07/12 23:45:26 wiz Exp $

PROG=		makeinfo
SRCS=		cmds.c defun.c files.c float.c footnote.c \
		html.c index.c insertion.c lang.c \
		macro.c makeinfo.c multi.c node.c \
		sectioning.c toc.c xml.c xref.c

CPPFLAGS+=	-I${IDIST}/makeinfo

.include <bsd.prog.mk>

.PATH: ${IDIST}/makeinfo ${IDIST}/doc
