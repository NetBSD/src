#	$NetBSD: Makefile,v 1.3 1997/04/28 23:46:20 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

PROG=	mkindex
HOST_CFLAGS+=-I${.CURDIR}/../boggle
NOMAN=	noman

# XXX for "make depend"
CFLAGS+=-I${.CURDIR}/../boggle

install:

# Override these rules for cross-compilation
.c.o:
	${HOST_COMPILE.c} ${.IMPSRC}

${PROG}: ${OBJS}
	${HOST_LINK.c} -o ${.TARGET} ${OBJS}

.include <bsd.prog.mk>
