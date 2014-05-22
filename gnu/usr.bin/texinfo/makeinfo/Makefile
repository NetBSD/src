#	$NetBSD: Makefile,v 1.7.4.1 2014/05/22 11:36:30 yamt Exp $

PROG=		makeinfo
SRCS=		cmds.c defun.c files.c float.c footnote.c \
		html.c index.c insertion.c lang.c \
		macro.c makeinfo.c multi.c node.c \
		sectioning.c toc.c xml.c xref.c

CPPFLAGS+=	-I${IDIST}/makeinfo

WARNS?=		0	# pointer sign issues

.include <bsd.prog.mk>

CPPFLAGS:=	${CPPFLAGS:S/-D_FORTIFY_SOURCE=2//}

.PATH: ${IDIST}/makeinfo ${IDIST}/doc
