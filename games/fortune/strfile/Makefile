#	$NetBSD: Makefile,v 1.7 1997/05/07 15:59:10 mycroft Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	strfile
MAN=	strfile.8

# only needed during build
proginstall::

# Override these rules for cross-compilation
.c.o:
	${HOST_COMPILE.c} ${.IMPSRC}

${PROG}: ${OBJS}
	${HOST_LINK.c} -o ${.TARGET} ${OBJS}

.include <bsd.prog.mk>
