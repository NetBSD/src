#	$NetBSD: Makefile,v 1.6 1997/05/07 15:59:08 mycroft Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

PROG=	mkdict
HOST_CFLAGS+=-I${.CURDIR}/../boggle
NOMAN=	noman

# XXX for "make depend"
CFLAGS+=-I${.CURDIR}/../boggle

# only needed during build
proginstall::

# Override these rules for cross-compilation
.c.o:
	${HOST_COMPILE.c} ${.IMPSRC}

${PROG}: ${OBJS}
	${HOST_LINK.c} -o ${.TARGET} ${OBJS}

.include <bsd.prog.mk>
