#	$NetBSD: Makefile,v 1.6 1997/04/28 23:53:25 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	strfile
MAN=	strfile.8

install:

# Override these rules for cross-compilation
.c.o:
	${HOST_COMPILE.c} ${.IMPSRC}

${PROG}: ${OBJS}
	${HOST_LINK.c} -o ${.TARGET} ${OBJS}

.include <bsd.prog.mk>
