#	$NetBSD: Makefile,v 1.1 1998/09/01 19:51:07 itohy Exp $

PROG=	aout2hux
MAN=	aout2hux.1

# only needed during build
proginstall::

# Override these rules for cross-compilation
.c.o:
	${HOST_COMPILE.c} ${.IMPSRC}

${PROG}: ${OBJS}
	${HOST_LINK.c} -o ${.TARGET} ${OBJS}

.include <bsd.prog.mk>

${OBJS}: aout68k.h hux.h type_local.h
