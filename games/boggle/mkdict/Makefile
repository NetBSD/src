#	$NetBSD: Makefile,v 1.7 1997/10/26 21:56:30 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

PROG=	mkdict
HOST_CFLAGS+=-I${.CURDIR}/../boggle
NOMAN=	noman

# XXX for "make depend"
CPPFLAGS+=-I${.CURDIR}/../boggle

# only needed during build
proginstall::

# Override these rules for cross-compilation
.c.o:
	${HOST_COMPILE.c} ${.IMPSRC}

${PROG}: ${OBJS}
	${HOST_LINK.c} -o ${.TARGET} ${OBJS}

.include <bsd.prog.mk>
